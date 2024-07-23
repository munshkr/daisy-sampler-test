#include <cstring>

#include "sample_reader.h"
#include "util/wav_format.h"
#include "fatfs_utils.h"
#include "logger.h"

using namespace daisy;

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

    requestNewSamples(FIFO_SIZE / 2);

    path_    = path;
    playing_ = true;
    invalid_ = false;

    return res;
}

FRESULT SampleReader::Close()
{
    playing_ = false;
    return close();
}

float SampleReader::Process()
{
    if(!playing_ || fifo_.IsEmpty())
    {
        return 0.0;
    }

    int16_t samp = fifo_.PopFront();

    // Load new samples if we're running low...
    // Threshold can be 1/8, 1/4, etc. (plan for worst case fill-time of all
    // voices if possible).

    const size_t num_samples_left = fifo_.GetNumElements();
    // LOG("[Process] %d samples left", num_samples_left);
    if(num_samples_left < LOAD_THRESHOLD && !HasPendingRequests()
       && f_eof(&fil_) == 0)
    {
        const size_t num_samples_to_request = FIFO_SIZE - num_samples_left;
        // LOG("[Process] Requesting %d new samples (only %d left)",
        //     num_samples_to_request,
        //     num_samples_left);
        requestNewSamples(num_samples_to_request);
    }

    return s162f(samp);
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
    req.temp_buffer = temp_buff_;
    PushRequest(req);
}

FRESULT SampleReader::Restart()
{
    FRESULT res = f_lseek(&fil_, data_pos_);
    if(res != FR_OK)
    {
        LOG_ERROR("[Restart]: Failed to seek to %d, result: %s",
                  data_pos_,
                  LogFsError(res));
    }
    else
    {
        LOG("[Restart]: Seeked to %d, result: %s", data_pos_, LogFsError(res));
    }

    // playing_ = true;

    requestNewSamples(FIFO_SIZE / 2);

    return res;
}

FRESULT SampleReader::close()
{
    path_     = "";
    data_pos_ = 0;
    invalid_  = true;
    return f_close(&fil_);
}
