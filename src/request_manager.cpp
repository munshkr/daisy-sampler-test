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

void RequestManager::HandleRequests()
{
    if(request_queue_.IsEmpty())
        return;

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
            FRESULT read_res = f_read(req.file,
                                      req.temp_buffer,
                                      req.num_samples * sizeof(int16_t),
                                      &bytes_read);
            if(read_res != FR_OK)
            {
                LOG_ERROR("[Read] Failed to read file: %s",
                          LogFsError(read_res));
            }

            // Push read samples into FIFO
            for(size_t i = 0; i < req.num_samples; i++)
            {
                req.fifo->PushBack(req.temp_buffer[i]);
            }

            break;
        }
        case Request::Type::Seek:
        {
            // TODO: do fseek,
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
