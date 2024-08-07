#pragma once

#include "daisy_seed.h"

using namespace daisy;

constexpr size_t FIFO_SIZE = 4096;

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
    size_t                    num_samples;
    FIFO<int16_t, FIFO_SIZE> *fifo;
    int16_t                  *temp_buffer;

    // Seek
    size_t seek_pos;
};

class RequestManager
{
  public:
    void PushRequest(const Request &req);
    bool HandleRequests();

  private:
    FIFO<Request, 32> request_queue_;
};
