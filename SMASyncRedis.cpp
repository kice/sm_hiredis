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

static void _cmdcb(void *data)
{
    CBData *d = (CBData *)data;
    IPluginFunction *cb = d->callback;
    if (cb->IsRunnable()) {
        HandleSecurity sec(d->identity, myself->GetIdentity());

        void *obj;
        int err = handlesys->ReadHandle(d->handle, g_AsyncRedisCtxType, &sec, &obj);
        if (err != HandleError_None || obj == nullptr) {
            delete d;
            return;
        }

        RedisReply *reply = new RedisReply();
        reply->reply = d->reply;

        HandleError error = HandleError_None;
        Handle_t qh = handlesys->CreateHandle(g_RedisReplyType, reply, d->identity, myself->GetIdentity(), &error);

        if (!qh || error != HandleError_None) {
            delete data;
            return;
        }

        cb->PushCell(d->handle);
        cb->PushCell(qh);
        cb->PushCell(d->data);
        cb->Execute(nullptr);

        if (qh != BAD_HANDLE) {
            handlesys->FreeHandle(qh, &sec);
        }
    }
    delete data;
}

void SMASyncRedis::CmdCallback(const redisAsyncContext *c, void *r, void *privdata)
{
    if (privdata) {
        CBData *data = (CBData *)privdata;
        data->reply = CopyReply((redisReply *)r);

        g_HiredisExt.RequestFrame(_cmdcb, privdata);
    }
}

static void _connectcb(void *data)
{
    CBData *d = (CBData *)data;
    IPluginFunction *cb = d->callback;
    if (cb && cb->IsRunnable()) {
        HandleSecurity sec;
        sec.pOwner = NULL;
        sec.pIdentity = myself->GetIdentity();
        void *obj;
        int err = handlesys->ReadHandle(d->handle, g_AsyncRedisCtxType, &sec, &obj);
        if (err != HandleError_None || obj == nullptr) {
            delete d;
            return;
        }

        cb->PushCell(d->handle);
        cb->PushCell(d->status);
        cb->PushCell(d->data);
        cb->Execute(nullptr);
    }

    delete d;
}

void SMASyncRedis::ConnectCallback(int status)
{
    ASyncRedis::ConnectCallback(status);

    CBData *cbdata = new CBData();
    cbdata->handle = handle;
    cbdata->callback = connectedCb;
    cbdata->data = connectData;
    cbdata->status = status;

    g_HiredisExt.RequestFrame(_connectcb, (void *)cbdata);
}

void SMASyncRedis::DisconnectCallback(int status)
{
    ASyncRedis::DisconnectCallback(status);

    CBData *cbdata = new CBData();
    cbdata->handle = handle;
    cbdata->callback = disconnectedCb;
    cbdata->data = disconnectData;
    cbdata->status = status;

    g_HiredisExt.RequestFrame(_connectcb, (void *)cbdata);
}
