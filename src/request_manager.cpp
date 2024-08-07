#include "request_manager.h"
#include "fatfs_utils.h"
#include "logger.h"

using namespace daisy;

void Requester::PushRequest(const Request& req)
{
    manager_->PushRequest(req);
    cnt_read_req_++;
}

void RequestManager::PushRequest(const Request& req)
{
    if(!request_queue_.PushBack(req))
    {
        LOG_ERROR("Request queue is full!");
    }
}

bool RequestManager::HandleRequest()
{
    if(request_queue_.IsEmpty())
        return false;

    auto req = request_queue_.PopFront();
    switch(req.type)
    {
        case Request::Type::Read:
        {
            // Read from file the number of samples requested into a temporary
            // buffer, then push them into the FIFO.
            size_t bytes_read = 0;

            // First read into temporary buffer
            // LOG("Reading %d samples from file", req.num_samples);
            size_t  bytes_to_read = req.num_samples * sizeof(int16_t);
            FRESULT read_res
                = f_read(req.file, req.temp_buffer, bytes_to_read, &bytes_read);
            if(read_res != FR_OK)
            {
                LOG_ERROR("[Read] Failed to read file: %s",
                          LogFsError(read_res));
            }

            // Push read samples into FIFO
            size_t samples_read = bytes_read / sizeof(int16_t);
            for(size_t i = 0; i < samples_read; i++)
            {
                req.fifo->PushBack(req.temp_buffer[i]);
            }

            break;
        }
        case Request::Type::Seek:
        {
            FRESULT res = f_lseek(req.file, req.seek_pos);
            if(res != FR_OK)
            {
                LOG_ERROR("[Seek] Failed to seek to %d, result: %s",
                          req.seek_pos,
                          LogFsError(res));
            }
            else
            {
                LOG("[Seek] Seeked to %d, result: %s",
                    req.seek_pos,
                    LogFsError(res));
            }
            break;
        }
        default:
        {
            LOG_ERROR("Unknown request type: %d", req.type);
            break;
        }
    }

    req.requester->AckRequest();
    return true;
}