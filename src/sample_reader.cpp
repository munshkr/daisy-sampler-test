#include <cstring>

#include "sample_reader.h"
#include "util/wav_format.h"
#include "fatfs_utils.h"
#include "logger.h"

using namespace daisy;

void SampleReader::Init(int16_t* buff, size_t buff_size)
{
    assert(buff_size_ % 2 == 0);

    buff_      = buff;
    buff_size_ = buff_size;
    path_      = "";

    // Reset the state, just in case
    read_ptr_       = 0;
    data_pos_       = 0;
    playing_        = false;
    looping_        = false;
    invalid_        = true;
    buff_state_     = BUFFER_STATE_IDLE;
    fade_out_count_ = 0;
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
    res = f_read(&fil_, (void*)&header, sizeof(WAV_FormatTypeDef), &bytesread);
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

    // If we were already playing another sample, start fading out. Calculate
    // count based on current read pointer and end of half-buffer (with
    // FADE_OUT_SAMPLES as max):
    if(playing_)
    {
        const size_t half_buffer_sz = buff_size_ / 2;
        fade_out_count_             = std::min(
            FADE_OUT_SAMPLES, half_buffer_sz - (read_ptr_ % half_buffer_sz));
        LOG("[Open] Fading out %d samples", fade_out_count_);
    }

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
    if(!playing_)
    {
        if(looping_)
            playing_ = true;
        return 0.0;
    }

    int16_t samp = buff_[read_ptr_];

    // If we are fading out, reduce gain of sample linearly
    if(fade_out_count_ > 0)
    {
        fade_out_count_--;
        const float ratio = fade_out_count_ / (float)FADE_OUT_SAMPLES;
        samp *= ratio;
        if(fade_out_count_ == 0)
        {
            last_sample_ = samp;
            samp         = 0.0;
        }
    }

    // Increment read pointer
    read_ptr_ = (read_ptr_ + 1) % buff_size_;
    if(read_ptr_ == 0)
    {
        buff_state_ = BUFFER_STATE_PREPARE_1;
    }
    else if(read_ptr_ == buff_size_ / 2)
    {
        buff_state_ = BUFFER_STATE_PREPARE_0;
    }

    // If we're changing buffers, force stop fading out. This shouldn't be
    // necessary, because count should be 0 already, but just in case...
    if(fade_out_count_ > 0 && (read_ptr_ == 0 || read_ptr_ == buff_size_ / 2))
    {
        fade_out_count_ = 0;
        samp            = 0.0;
    }

    return s162f(samp);
}

FRESULT SampleReader::Prepare()
{
    if(last_sample_ != INVALID_SAMPLE)
    {
        LOG("[Prepare] Last sample before fade out: %f", last_sample_);
        last_sample_ = INVALID_SAMPLE;
    }

    if(!playing_ || invalid_)
        return FR_OK;

    if(buff_state_ != BUFFER_STATE_IDLE)
    {
        size_t offset, bytesread, rxsize;
        bytesread = 0;
        rxsize    = (buff_size_ / 2) * sizeof(buff_[0]);
        offset    = buff_state_ == BUFFER_STATE_PREPARE_1 ? buff_size_ / 2 : 0;
        FRESULT read_res = f_read(&fil_, &buff_[offset], rxsize, &bytesread);
        if(read_res != FR_OK)
        {
            LOG_ERROR("[Prepare] Failed to read file %s: %s",
                      path_.c_str(),
                      LogFsError(read_res));
            return read_res;
        }

        if(bytesread < rxsize || f_eof(&fil_))
        {
            LOG("[Prepare] Reached end of file %s", path_.c_str());
            if(looping_)
            {
                LOG("[Prepare] Restarting file %s", path_.c_str());
                Restart();
            }
            else
            {
                playing_ = false;
            }
        }

        buff_state_ = BUFFER_STATE_IDLE;
    }

    return FR_OK;
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

    // If we were already playing another sample, start fading out. Calculate
    // count based on current read pointer and end of half-buffer (with
    // FADE_OUT_SAMPLES as max):
    const size_t half_buffer_sz = buff_size_ / 2;
    fade_out_count_             = std::min(FADE_OUT_SAMPLES,
                               half_buffer_sz - (read_ptr_ % half_buffer_sz));
    LOG("[Open] Fading out %d samples", fade_out_count_);
    // playing_ = true;

    return res;
}

FRESULT SampleReader::close()
{
    path_     = "";
    data_pos_ = 0;
    invalid_  = true;
    return f_close(&fil_);
}
