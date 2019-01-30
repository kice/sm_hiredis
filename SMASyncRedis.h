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

#include "ASyncRedis.h"

#include "smsdk_ext.h"
#include "concurrentqueue.h"

#include <vector>
#include <functional>

struct CBData
{
    Handle_t handle;
    int status;
    IPluginFunction *callback;
    IdentityToken_t *identity;
    redisReply *reply;
    cell_t data;
};

class SMASyncRedis :public ASyncRedis
{
public:
    SMASyncRedis(int intval = 1000, int cache_size = 32) :ASyncRedis(intval, cache_size) {}

    IPluginFunction *connectedCb;
    IPluginFunction *disconnectedCb;

    cell_t connectData;
    cell_t disconnectData;

    Handle_t handle;

    void CmdCallback(const redisAsyncContext *c, void *r, void *privdata);

    void ConnectCallback(int status);
    void DisconnectCallback(int status);
};
