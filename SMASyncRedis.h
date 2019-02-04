#pragma once

extern "C" {
#ifdef _WIN32
#define WIN32_INTEROP_APIS_H
#define NO_QFORKIMPL
#include <Win32_Interop/win32fixes.h>

#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")
#endif
}

#include "client.h"

#include "smsdk_ext.h"
#include "concurrentqueue.h"

#include <vector>
#include <functional>

struct redisReply;

struct CBData
{
    Handle_t handle;
    int status;
    IPluginFunction *callback;
    IdentityToken_t *identity;
    redisReply *reply;
    cell_t data;
};

class SMASyncRedis :public async_redis::client
{
public:
    SMASyncRedis(size_t piped_cache = 0, uint32_t pipeline_timeout = 0) 
        : async_redis::client(piped_cache, pipeline_timeout) {}

    Handle_t handle;

    void CmdCallback(void *r, void *privdata);
};
