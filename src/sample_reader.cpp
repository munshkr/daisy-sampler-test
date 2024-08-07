#include <cstring>

#include "sample_reader.h"
#include "util/wav_format.h"
#include "fatfs_utils.h"
#include "logger.h"

using namespace daisy;

void SampleReader::Init()
{
    fifo_.Init();
}

FRESULT SampleReader::Open(std::string path)
{
    // NOTE: This is an optimization ot avoid re-opening the same file, but it
    // is commented out now because we want to compare in this example:
    // if(path_ == path)
    // {
    //     Restart();
    //     return FR_OK;
    // }

    close();

    FRESULT res;
    // Open file
    res = f_open(&fil_, path.c_str(), (FA_OPEN_EXISTING | FA_READ));
    if(res != FR_OK)
    {
        LOG_ERROR(
            "[Open] Failed to open file %s: %s", path.c_str(), LogFsError(res));
        return res;
    }
    LOG("[Open] Opened file %s", path.c_str());

    // Read WAV header
    WAV_FormatTypeDef header;
    size_t            bytesread;
    res = f_read(&fil_, (void *)&header, sizeof(WAV_FormatTypeDef), &bytesread);
    if(res != FR_OK)
    {
        LOG_ERROR("[Init] Failed to read WAV info from %s", path.c_str());
        return res;
    }
    // Store data chunk position
    data_pos_ = sizeof(WAV_FormatTypeDef) + header.SubChunk1Size;

    // Seek to data position
    res = f_lseek(&fil_, data_pos_);
    if(res != FR_OK)
    {
        LOG_ERROR(
            "[Open] Failed to seek to %d: %s", data_pos_, LogFsError(res));
        return res;
    }
    LOG("[Open] Seeked to %d: %s", data_pos_, LogFsError(res));

    path_    = path;
    playing_ = true;
    invalid_ = false;

    requestNewSamples(FIFO_SIZE / 2);
    awaiting_start_ = true;
    fifo_.Flush();

    return res;
}

FRESULT SampleReader::Close()
{
    playing_ = false;
    return close();
}

float SampleReader::Process()
{
    if(!playing_)
    {
        return 0.0;
    }

    if(fifo_.isEmpty())
    {
        if(!awaiting_start_)
        {
            underrun_samples_++;
            underrun_total_samples++;
        }
        return 0.0;
    }

    awaiting_start_ = false;

    if(underrun_samples_ > 0)
    {
        underruns++;
        underrun_samples_ = 0;
    }

    int16_t samp = fifo_.ImmediateRead();

    // Load new samples if we're running low...
    // Threshold can be 1/8, 1/4, etc. (plan for worst case fill-time of all
    // voices if possible).

    const size_t num_samples_left = fifo_.readable();
    // LOG("[Process] %d samples left", num_samples_left);
    if(num_samples_left < LOAD_THRESHOLD && !HasPendingRequests()
       && f_eof(&fil_) == 0)
    {
        const size_t num_samples_to_request
            = FIFO_SIZE / 2; // FIFO_SIZE - num_samples_left;
        // LOG("[Process] Requesting %d new samples (only %d left)",
        //     num_samples_to_request,
        //     num_samples_left);
        requestNewSamples(num_samples_to_request);
    }

    return s162f(samp);
}

void SampleReader::Process(float *out, size_t size)
{
    if(!playing_)
    {
        std::fill(out, out + size, 0.0f);
        return;
    }

    for(size_t i = 0; i < size; i++)
    {
        if(fifo_.isEmpty())
        {
            if(!awaiting_start_)
            {
                underrun_samples_++;
                underrun_total_samples++;
            }
            out[i] = 0.0f;
            continue;
        }

        awaiting_start_ = false;
        out[i] = s162f(fifo_.ImmediateRead());
    }

    // Load new samples if we're running low...
    // Threshold can be 1/8, 1/4, etc. (plan for worst case fill-time of all
    // voices if possible).

    const size_t num_samples_left = fifo_.readable();
    // LOG("[Process] %d samples left", num_samples_left);
    if(num_samples_left < LOAD_THRESHOLD && !HasPendingRequests()
       && f_eof(&fil_) == 0)
    {
        const size_t num_samples_to_request
            = FIFO_SIZE / 2; // FIFO_SIZE - num_samples_left;
        // LOG("[Process] Requesting %d new samples (only %d left)",
        //     num_samples_to_request,
        //     num_samples_left);
        requestNewSamples(num_samples_to_request);
    }

    if(underrun_samples_ > 0)
    {
        underruns++;
        underrun_samples_ = 0;
    }
}


void SampleReader::Restart()
{
    InvalidatePendingRequests();
    requestRestart();
    requestNewSamples(FIFO_SIZE / 2);
    // TODO: should handle immediate re-seek better than this - fade out, etc
    fifo_.Flush();
    awaiting_start_ = true;
}

FRESULT SampleReader::close()
{
    path_     = "";
    data_pos_ = 0;
    invalid_  = true;
    return f_close(&fil_);
}

void SampleReader::requestNewSamples(const size_t num_samples)
{
    if(num_samples == 0)
    {
        LOG_ERROR("[requestNewSamples] Cannot request 0 samples!");
        return;
    }

    Request req;
    req.type        = Request::Type::Read;
    req.requester   = this;
    req.file        = &fil_;
    req.num_samples = num_samples;
    req.fifo        = &fifo_;
    PushRequest(req);
}

void SampleReader::requestRestart()
{
    Request req;
    req.type      = Request::Type::Seek;
    req.requester = this;
    req.file      = &fil_;
    req.seek_pos  = data_pos_;
    PushRequest(req);
}
