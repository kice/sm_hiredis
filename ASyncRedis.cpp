#pragma once

#include "ASyncRedis.h"

#include <vector>
#include <string>

struct QueryCb
{
    ASyncRedis::CommandCallback cb;
    void *data;
};

static void cmdCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = (redisReply *)r;
    QueryCb *cb = (QueryCb *)privdata;
    if (cb) {
        if (cb->cb) {
            cb->cb(c, r, cb->data);
        }
        delete cb;
    }
}

static void connectCallback(const redisAsyncContext *c, int status)
{
    if (c && c->data) {
        ((ASyncRedis *)c->data)->ConnectCallback(status);
    }
}

static void disconnectCallback(const redisAsyncContext *c, int status)
{
    if (c && c->data) {
        ((ASyncRedis *)c->data)->DisconnectCallback(status);
    }
}

static int timeEventCallback(aeEventLoop *eventLoop, PORT_LONGLONG id, void *clientData)
{
    auto ac = (ASyncRedis *)clientData;
    return ac->EventMain();
}

ASyncRedis::ASyncRedis(int _intval, int cache)
{
    connected = false;
    disconnected = false;

    interval = _intval;
    cache_size = cache;

    loop = aeCreateEventLoop(1024 * 10);
    aeCreateTimeEvent(loop, interval, timeEventCallback, this, nullptr);

    evMain = std::thread([this] {
        aeMain(loop);
        aeDeleteEventLoop(loop);
        loop = nullptr;
    });

    std::this_thread::yield();
}

ASyncRedis::~ASyncRedis()
{
    if (loop) {
        aeStop(loop);
    }

    if (evMain.joinable()) {
        evMain.join();
    }

    if (!disconnected && ac) {
        redisAsyncDisconnect(ac);
    }
}

bool ASyncRedis::WaitForConnected()
{
    std::unique_lock<std::mutex> lk(lock);
    cv_connect.wait(lk, [this] {return connected; });
    return connected;
}

bool ASyncRedis::WaitForDisconnected()
{
    std::unique_lock<std::mutex> lk(lock);
    cv_disconnect.wait(lk, [this] {return disconnected; });
    return disconnected;
}

bool ASyncRedis::Connect(const char *ip, int port)
{
    connected = false;
    disconnected = false;
    ac = redisAsyncConnect(ip, port);
    if (ac->err) {
        disconnected = true;
        return false;
    }
    return Attach();
}

bool ASyncRedis::ConnectBind(const char *ip, int port, const char *source_addr)
{
    connected = false;
    disconnected = false;
    ac = redisAsyncConnectBind(ip, port, source_addr);
    if (ac->err) {
        disconnected = true;
        return false;
    }
    return Attach();
}

bool ASyncRedis::Attach()
{
    ac->data = this;

    if (redisAeAttach(loop, ac) != REDIS_OK) {
        return false;
    }

    if (redisAsyncSetConnectCallback(ac, connectCallback) != REDIS_OK) {
        return false;
    }

    if (redisAsyncSetDisconnectCallback(ac, disconnectCallback) != REDIS_OK) {
        return false;
    }

    return true;
}

bool ASyncRedis::Command(const char *cmd)
{
    if (!disconnected) {
        return cmdlist.try_enqueue({ cmd, nullptr });
    }
    return false;
}

bool ASyncRedis::Command(const char *cmd, CommandCallback cb, void *data)
{
    if (!disconnected) {
        QueryCb *p = new QueryCb{ cb, data };
        return cmdlist.try_enqueue({ cmd, p });
    }
    return false;
}

void ASyncRedis::ConnectCallback(int status)
{
    std::unique_lock <std::mutex> lck(lock);
    connected = true;
    cv_connect.notify_all();

    if (status != REDIS_OK) {
        aeStop(loop);
        return;
    }
}

void ASyncRedis::DisconnectCallback(int status)
{
    std::unique_lock <std::mutex> lck(lock);
    disconnected = true;
    cv_disconnect.notify_all();

    if (status != REDIS_OK) {
        aeStop(loop);
        return;
    }
}

int ASyncRedis::EventMain()
{
    if (cmdlist.size_approx()) {
        std::vector<QueryOp> cmds(cache_size);
        int n = cmdlist.try_dequeue_bulk(cmds.begin(), cmds.size());
        for (int i = 0; i < n; ++i) {
            auto[cmd, cb] = cmds[i];
            if (cb) {
                redisAsyncCommand(ac, cmdCallback, cb, cmd.c_str());
            } else {
                redisAsyncCommand(ac, nullptr, nullptr, cmd.c_str());
            }
        }
    }
    return interval;
}
