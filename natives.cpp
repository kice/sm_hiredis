/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Client Preferences Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"

#include <string>
#include <sstream>
#include <vector>

template<class _Elem, class _Traits, class _Alloc>
inline std::vector<std::basic_string<_Elem, _Traits, _Alloc>> split(
    const std::basic_string<_Elem, _Traits, _Alloc>& _Str, const _Elem _Delim)
{
    std::vector<std::basic_string<_Elem, _Traits, _Alloc>> elems;
    std::basic_stringstream<_Elem, _Traits, _Alloc> ss(_Str);
    std::basic_string<_Elem, _Traits, _Alloc> item;
    while (std::getline(ss, item, _Delim)) {
        elems.push_back(item);
    }
    return elems;
}

template <typename T>
static bool ReadHandle(const cell_t handle, HandleType_t type, T** obj, IPluginContext *pContext)
{
    Handle_t hndl = static_cast<Handle_t>(handle);
    HandleError err;
    HandleSecurity sec;
    sec.pOwner = NULL;
    sec.pIdentity = myself->GetIdentity();

    err = handlesys->ReadHandle(hndl, type, &sec, (void **)obj);
    if (err != HandleError_None) {
        pContext->ThrowNativeError("Invalid handle %x (error %d)", hndl, err);
        return false;
    }

    if (*obj == nullptr) {
        pContext->ThrowNativeError("Invalid %x (handle == nullprt)", hndl);
        return false;
    }

    return true;
}

static inline std::vector<std::string> formatCommand(const std::string &cmd)
{
    return split(cmd, ' ');
}

static inline void *redisBlockForReply(redisContext *c)
{
    void *reply;

    if (c->flags & REDIS_BLOCK) {
        if (redisGetReply(c, &reply) != REDIS_OK)
            return NULL;
        return reply;
    }
    return NULL;
}

// Blocking API

static cell_t native_redis(IPluginContext *pContext, const cell_t *params)
{
    auto ctx = new RedisConnect();
    if (ctx == nullptr) {
        pContext->ReportError("Cannot create redis connection");
        return 0;
    }

    ctx->ctx = nullptr;

    HandleError error = HandleError_None;
    Handle_t handle = handlesys->CreateHandle(g_RedisCtxType, (void *)ctx, pContext->GetIdentity(), myself->GetIdentity(), &error);

    if (!handle || error != HandleError_None) {
        delete ctx;
        pContext->ReportError("Allocation of Redis handle failed, error code #%d", error);
        return 0;
    }
    return handle;
}

static cell_t native_redisConnect(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char *ip;
    pContext->LocalToString(params[2], &ip);

    ctx->ctx = redisConnect(ip, params[3]);
    if (ctx->ctx == nullptr) {
        pContext->ReportError("Cannot create redis connection");
        return 0;
    }

    return 1;
}

static cell_t native_redisConnectWithTimeout(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char *ip;
    pContext->LocalToString(params[2], &ip);

    struct timeval timeout = { params[4], 0 };
    ctx->ctx = redisConnectWithTimeout(ip, params[3], timeout);
    if (ctx->ctx == nullptr) {
        pContext->ReportError("Cannot create redis connection");
        return 0;
    }

    return 1;
}

static cell_t native_redisGetReply(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    void *reply = nullptr;
    redisGetReply(ctx->ctx, &reply);
    if (reply == nullptr) {
        return 0;
    }

    RedisReply *r = new RedisReply();
    r->reply = (redisReply *)reply;

    HandleError error = HandleError_None;
    Handle_t handle = handlesys->CreateHandle(g_RedisReplyType, r, pContext->GetIdentity(), myself->GetIdentity(), &error);

    if (!handle || error != HandleError_None) {
        pContext->ReportError("Allocation of RedisReply handle failed, error code #%d", error);
        return 0;
    }
    return handle;
}

static cell_t native_redisAppendCommand(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char buffer[1024];
    {
        DetectExceptions eh(pContext);
        g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
        if (eh.HasException()) {
            return 0;
        }
    }

    return redisAppendCommand(ctx->ctx, buffer) == REDIS_OK;
}

static cell_t native_redisCommand(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char buffer[1024];
    {
        DetectExceptions eh(pContext);
        g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
        if (eh.HasException()) {
            return 0;
        }
    }

    if (redisAppendCommand(ctx->ctx, buffer) != REDIS_OK) {
        return 0;
    }

    void *reply = redisBlockForReply(ctx->ctx);
    if (reply == nullptr) {
        return 0;
    }

    RedisReply *r = new RedisReply();
    r->reply = (redisReply *)reply;

    HandleError error = HandleError_None;
    Handle_t handle = handlesys->CreateHandle(g_RedisReplyType, r, pContext->GetIdentity(), myself->GetIdentity(), &error);

    if (!handle || error != HandleError_None) {
        pContext->ReportError("Allocation of RedisReply handle failed, error code #%d", error);
        return 0;
    }
    return handle;
}

static cell_t native_redisGetError(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    return ctx->ctx->err;
}

static cell_t native_redisGetErrorStr(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    size_t numWritten = 0;
    pContext->StringToLocalUTF8(params[2], params[3], ctx->ctx->errstr, &numWritten);
    return numWritten;
}

static cell_t native_redisSetTimeout(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    struct timeval timeout = { params[2], 0 };
    return redisSetTimeout(ctx->ctx, timeout) == REDIS_OK;
}

static cell_t native_redisEnableKeepAlive(IPluginContext *pContext, const cell_t *params)
{
    RedisConnect *ctx = nullptr;
    if (!ReadHandle(params[1], g_RedisCtxType, &ctx, pContext)) {
        return 0;
    }

    return redisEnableKeepAlive(ctx->ctx) == REDIS_OK;
}

// Non-blocking API

static cell_t native_redisAsync(IPluginContext *pContext, const cell_t *params)
{
    auto ctx = new SMASyncRedis(params[1], params[2]);
    if (ctx == nullptr) {
        pContext->ReportError("Cannot create async redis connection");
        return 0;
    }

    HandleError error = HandleError_None;
    Handle_t handle = handlesys->CreateHandle(g_AsyncRedisCtxType, ctx, pContext->GetIdentity(), myself->GetIdentity(), &error);

    if (!handle || error != HandleError_None) {
        delete ctx;
        pContext->ReportError("Allocation of async Redis handle failed, error code #%d", error);
        return 0;
    }

    ctx->handle = handle;
    return handle;
}

static cell_t native_redisAsyncConnect(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (!ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char *ip;
    pContext->LocalToString(params[2], &ip);
    return ctx->Connect(ip, params[3], params[4]);
}

static cell_t native_redisAsyncIsConnected(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        return ctx->IsConnected();
    }
    return 0;
}

static cell_t native_redisAsyncDisconnect(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        ctx->Disconnect();
    }
    return 0;
}

static cell_t native_redisAsyncAppend(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (!ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char buffer[1024];
    {
        DetectExceptions eh(pContext);
        g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 4);
        if (eh.HasException()) {
            return 0;
        }
    }

    auto cb = pContext->GetFunctionById(params[2]);
    if (cb->IsRunnable()) {
        CBData *cbdata = new CBData();
        cbdata->handle = params[1];
        cbdata->callback = cb;
        cbdata->data = params[3];
        cbdata->identity = pContext->GetIdentity();

        ctx->Append(formatCommand(buffer), [=](void *reply) {
            ctx->CmdCallback(reply, cbdata);
        });
    } else {
        ctx->Append(formatCommand(buffer));
    }
    return params[1];
}

static cell_t native_redisAsyncCommand(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (!ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        return 0;
    }

    char buffer[1024];
    {
        DetectExceptions eh(pContext);
        g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
        if (eh.HasException()) {
            return 0;
        }
    }

    Handle_t handle = 0;

    auto reply = ctx->Command<redisReply>(formatCommand(buffer)).get();
    if (reply) {
        RedisReply *r = new RedisReply();
        r->reply = reply;

        HandleError error = HandleError_None;
        handle = handlesys->CreateHandle(g_RedisReplyType, r, pContext->GetIdentity(), myself->GetIdentity(), &error);

        if (!handle || error != HandleError_None) {
            pContext->ReportError("Allocation of RedisReply handle failed, error code #%d", error);
            return 0;
        }
    }
    return handle;
}

static cell_t native_redisAsyncCommit(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        ctx->Commit();
        return params[1];
    }
    return 0;
}

static cell_t native_redisAsyncGetError(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (!ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        return -1;
    }
    return ctx->GetError();
}

static cell_t native_redisAsyncGetErrorStr(IPluginContext *pContext, const cell_t *params)
{
    SMASyncRedis *ctx = nullptr;
    if (!ReadHandle(params[1], g_AsyncRedisCtxType, &ctx, pContext)) {
        return 0;
    }

    size_t numWritten = 0;
    pContext->StringToLocalUTF8(params[2], params[3], ctx->GetErrorString(), &numWritten);
    return numWritten;
}

// Redis reply

static cell_t native_redisReplyType(IPluginContext *pContext, const cell_t *params)
{
    RedisReply *r = nullptr;
    if (!ReadHandle(params[1], g_RedisReplyType, &r, pContext)) {
        return 0;
    }
    redisReply *reply = r->reply;

    return reply->type;
}

static cell_t native_redisReplyGetString(IPluginContext *pContext, const cell_t *params)
{
    RedisReply *r = nullptr;
    if (!ReadHandle(params[1], g_RedisReplyType, &r, pContext)) {
        return 0;
    }
    redisReply *reply = r->reply;

    if (reply->str == nullptr) {
        return 0;
    }

    size_t numWritten = 0;
    pContext->StringToLocalUTF8(params[2], params[3], reply->str, &numWritten);
    return numWritten;
}

static cell_t native_redisReplyGetInt(IPluginContext *pContext, const cell_t *params)
{
    RedisReply *r = nullptr;
    if (!ReadHandle(params[1], g_RedisReplyType, &r, pContext)) {
        return 0;
    }
    redisReply *reply = r->reply;

    return reply->integer;
}

static cell_t native_redisReplyGetArraySize(IPluginContext *pContext, const cell_t *params)
{
    RedisReply *r = nullptr;
    if (!ReadHandle(params[1], g_RedisReplyType, &r, pContext)) {
        return 0;
    }
    redisReply *reply = r->reply;

    return reply->elements;
}

static cell_t native_redisReplyGetElement(IPluginContext *pContext, const cell_t *params)
{
    RedisReply *r = nullptr;
    if (!ReadHandle(params[1], g_RedisReplyType, &r, pContext)) {
        return 0;
    }
    redisReply *reply = r->reply;

    size_t idx = params[2];
    if (idx >= reply->elements || reply->element == nullptr) {
        return pContext->ThrowNativeError("Invalid index for redis reply array: %d (count: %d)", idx, reply->elements);
    }

    redisReply *element = reply->element[idx];
    if (element == nullptr) {
        return 0;
    }

    RedisReply *_ele = new RedisReply();
    _ele->reply = element;
    _ele->flag = false;

    HandleError error = HandleError_None;
    HandleType_t handle = handlesys->CreateHandle(g_RedisReplyType, _ele, pContext->GetIdentity(), myself->GetIdentity(), &error);

    if (!handle || error != HandleError_None) {
        pContext->ReportError("Allocation of RedisReply handle failed, error code #%d", error);
        return 0;
    }
    return handle;
}

sp_nativeinfo_t g_RedisNatives[] =
{
    // blocking connection
    { "Redis.Redis",                native_redis },
    { "Redis.Connect",              native_redisConnect },
    { "Redis.ConnectWithTimeout",   native_redisConnectWithTimeout },
    { "Redis.GetReply",             native_redisGetReply },
    { "Redis.AppendCommand",        native_redisAppendCommand },
    { "Redis.Command",              native_redisCommand },
    { "Redis.Error.get",            native_redisGetError },
    { "Redis.GetErrorString",       native_redisGetErrorStr },
    { "Redis.SetTimeout",           native_redisSetTimeout },
    { "Redis.EnableKeepAlive",      native_redisEnableKeepAlive },

    // non-blocking connection
    { "RedisAsync.RedisAsync",              native_redisAsync },
    { "RedisAsync.Connect",                 native_redisAsyncConnect },
    { "RedisAsync.IsConnected.get",         native_redisAsyncIsConnected },
    { "RedisAsync.Disconnect",              native_redisAsyncDisconnect },
    { "RedisAsync.Append",                  native_redisAsyncAppend },
    { "RedisAsync.Command",                 native_redisAsyncCommand },
    { "RedisAsync.Commit",                  native_redisAsyncCommit },
    { "RedisAsync.Error.get",               native_redisAsyncGetError },
    { "RedisAsync.GetErrorString",          native_redisAsyncGetErrorStr },

    // Reply API
    { "RedisReply.Type.get",        native_redisReplyType },
    { "RedisReply.GetString",       native_redisReplyGetString },
    { "RedisReply.IntValue.get",    native_redisReplyGetInt },
    { "RedisReply.ArraySize.get",   native_redisReplyGetArraySize },
    { "RedisReply.GetElement",      native_redisReplyGetElement },

    { nullptr,							nullptr }
};
