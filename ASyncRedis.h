#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <tuple>
#include <functional>

#include "concurrentqueue.h"

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
extern "C" {
#include "adapters/ae.h"
}

class ASyncRedis
{
public:
    using CommandCallback = std::function<void(const redisAsyncContext *c, void *r, void *privdata)>;
    using CommandCallback = std::function<void(const redisAsyncContext *c, void *r, void *privdata)>;
    using CommandCallback = std::function<void(const redisAsyncContext *c, void *r, void *privdata)>;

    ASyncRedis(int intval = 1000, int cache_size = 32);
    ~ASyncRedis();

    bool Connect(const char *ip, int port);
    bool ConnectBind(const char *ip, int port, const char *source_addr);

    bool WaitForConnected();
    bool WaitForDisconnected();

    bool Command(const char *cmd);
    bool Command(const char *cmd, CommandCallback cb, void *data);

    virtual void ConnectCallback(int status);
    virtual void DisconnectCallback(int status);

    int EventMain();

    const redisAsyncContext* GetContext() const
    {
        return ac;
    }

private:
    using QueryOp = std::tuple<std::string, void *>;
    using QueryOps = moodycamel::ConcurrentQueue<QueryOp>;

    bool Attach();

    int interval;
    int cache_size;

    aeEventLoop *loop;
    redisAsyncContext *ac;
    std::thread evMain;

    std::mutex lock;
    std::condition_variable cv_connect;
    std::condition_variable cv_disconnect;

    bool connected;
    bool disconnected;

    QueryOps cmdlist;
};
