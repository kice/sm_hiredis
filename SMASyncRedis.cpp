#include "SMASyncRedis.h"
#include "extension.h"

static inline redisReply *CopyReply(redisReply *src)
{
    redisReply *reply = (redisReply *)calloc(1, sizeof(redisReply));
    reply->type = src->type;
    reply->integer = src->integer;

    switch (reply->type) {
    case REDIS_REPLY_INTEGER:
        break;
    case REDIS_REPLY_ARRAY:
        if (src->element != nullptr) {
            reply->elements = src->elements;
            reply->element = (redisReply **)calloc(src->elements, sizeof(redisReply *));
            for (size_t i = 0; i < src->elements; i++) {
                if (src->element[i] != nullptr) {
                    reply->element[i] = CopyReply(src->element[i]);
                }
            }
        }
        break;
    case REDIS_REPLY_ERROR:
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_STRING:
        if (src->str != nullptr) {
            reply->str = (char *)calloc(src->len + 1, sizeof(char));
            memcpy(reply->str, src->str, src->len);
        }
    }
    return reply;
}

void SMASyncRedis::CmdCallback(const redisAsyncContext *c, void *r, void *privdata)
{
    if (privdata) {
        CmdCBData *data = (CmdCBData *)privdata;
        data->reply = CopyReply((redisReply *)r);

        OutputDebugString(std::to_string((int)data->reply).c_str());

        g_HiredisExt.RequestFrame(std::bind(&SMASyncRedis::_cmdcb, this, std::placeholders::_1), privdata);
    }
}

void SMASyncRedis::_cmdcb(void *data)
{
    CmdCBData *d = (CmdCBData *)data;
    IPluginFunction *cb = d->callback;
    if (cb->IsRunnable()) {
        HandleSecurity sec(d->identity, myself->GetIdentity());

        RedisReply *reply = new RedisReply();
        reply->reply = d->reply;

        HandleError error = HandleError_None;
        Handle_t qh = handlesys->CreateHandle(g_RedisReplyType, reply, d->identity, myself->GetIdentity(), &error);

        if (!qh || error != HandleError_None) {
            delete data;
            return;
        }

        cb->PushCell(handle);
        cb->PushCell(qh);
        cb->PushCell(d->data);
        cb->Execute(nullptr);

        if (qh != BAD_HANDLE) {
            handlesys->FreeHandle(qh, &sec);
        }
    }
    delete data;
}

void SMASyncRedis::ConnectCallback(int status)
{
    ASyncRedis::ConnectCallback(status);
    g_HiredisExt.RequestFrame(std::bind(&SMASyncRedis::_connectcb, this, std::placeholders::_1), (void *)status);
}

void SMASyncRedis::_connectcb(void *data)
{
    int status = (int)data;
    if (connectedCb && connectedCb->IsRunnable()) {
        connectedCb->PushCell(handle);
        connectedCb->PushCell(status);
        connectedCb->PushCell(connectData);
        connectedCb->Execute(nullptr);
    }
}

void SMASyncRedis::DisconnectCallback(int status)
{
    ASyncRedis::DisconnectCallback(status);
    g_HiredisExt.RequestFrame(std::bind(&SMASyncRedis::_disconnectcb, this, std::placeholders::_1), (void *)status);
}

void SMASyncRedis::_disconnectcb(void *data)
{
    int status = (int)data;
    if (disconnectedCb && disconnectedCb->IsRunnable()) {
        disconnectedCb->PushCell(handle);
        disconnectedCb->PushCell(status);
        disconnectedCb->PushCell(disconnectData);
        disconnectedCb->Execute(nullptr);
    }
}
