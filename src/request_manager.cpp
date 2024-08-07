#include "request_manager.h"
#include "fatfs_utils.h"
#include "logger.h"

using namespace daisy;

void Requester::PushRequest(const Request& req)
{
    manager_->PushRequest(req);
    cnt_read_req_++;
}

void Requester::InvalidatePendingRequests()
{
    cnt_read_req_ -= manager_->InvalidatePendingRequests(this);
}

void RequestManager::PushRequest(const Request& req)
{
    if(!request_queue_.PushBack(req))
    {
        LOG_ERROR("Request queue is full!");
    }
}

bool RequestManager::HandleRequests()
{
    if(request_queue_.IsEmpty())
        return false;

    while(!request_queue_.IsEmpty())
    {
        auto req = request_queue_.PopFront();
        if(!req.valid)
            continue;
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
                FRESULT read_res      = f_read(
                    req.file, temp_buffer_, bytes_to_read, &bytes_read);
                if(read_res != FR_OK)
                {
                    LOG_ERROR("[Read] Failed to read file: %s",
                              LogFsError(read_res));
                }

                // Push read samples into FIFO
                size_t samples_read = bytes_read / sizeof(int16_t);
                req.fifo->Overwrite(temp_buffer_, samples_read);
                // for(size_t i = 0; i < samples_read; i++)
                // {
                //     req.fifo->Write(req.temp_buffer[i]);
                // }

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
    }
    return true;
}

size_t RequestManager::InvalidatePendingRequests(Requester* requester)
{
    // prevent any new requests from ISR while we're doing this
    ScopedIrqBlocker block;
    size_t           count = 0;
    for(size_t i = 0; i < request_queue_.GetNumElements(); i++)
    {
        if(request_queue_[i].requester == requester)
        {
            request_queue_[i].valid = false;
            count++;
        }
    }
    return count;
}
