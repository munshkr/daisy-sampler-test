#pragma once

#include "daisy_seed.h"

using namespace daisy;

class RequestManager;
struct Request;

class Requester
{
  public:
    void SetRequestManager(RequestManager *manager) { manager_ = manager; }
    void AckRequest() { cnt_read_req_--; }
    bool HasPendingRequests() const { return cnt_read_req_ > 0; }
    void PushRequest(const Request &req);

  private:
    RequestManager *manager_      = nullptr;
    size_t          cnt_read_req_ = 0;
};

struct Request
{
    enum class Type
    {
        Read,
        Seek,
    };

    Type       type;
    Requester *requester;
    FIL       *file;

    // Read
    size_t   num_samples;
    int16_t *buffer;
    size_t   buffer_size;
    int16_t *temp_buffer;
    size_t  *loaded_ptr;
};

class RequestManager
{
  public:
    void PushRequest(const Request &req);
    void HandleRequests();

  private:
    FIFO<Request, 32> request_queue_;
};
