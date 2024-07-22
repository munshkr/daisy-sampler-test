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
    request_queue_.PushBack(req);
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
            LOG("Reading %d samples from file", req.num_samples);
            FRESULT read_res = f_read(req.file,
                                      req.temp_buffer,
                                      req.num_samples * sizeof(int16_t),
                                      &bytes_read);
            if(read_res != FR_OK)
            {
                LOG_ERROR("[Read] Failed to read file: %s",
                          LogFsError(read_res));
            }

            // Copy from temporary buffer to the main buffer.

            // If num_samples is greater than the number of samples left in the
            // buffer, then we need to do two memcpy's, one for the remaining
            // samples in the buffer, and a second one for the remaining samples
            // in the temporary buffer into the beginning of the buffer.
            if(bytes_read > 0)
            {
                size_t samples_to_copy = bytes_read / sizeof(int16_t);
                size_t samples_left    = req.buffer_size - *req.loaded_ptr;

                if(samples_to_copy > samples_left)
                {
                    // Copy the remaining samples in the buffer
                    memcpy(req.buffer + *req.loaded_ptr,
                           req.temp_buffer,
                           samples_left * sizeof(int16_t));

                    // Copy the remaining samples in the temporary buffer
                    memcpy(req.buffer,
                           req.temp_buffer + samples_left,
                           (samples_to_copy - samples_left) * sizeof(int16_t));
                }
                else
                {
                    // Copy all samples into the buffer
                    memcpy(req.buffer + *req.loaded_ptr,
                           req.temp_buffer,
                           samples_to_copy * sizeof(int16_t));
                }
            }

            // Update loaded_ptr
            *req.loaded_ptr = (bytes_read * sizeof(int16_t)) % req.buffer_size;

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
