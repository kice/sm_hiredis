/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
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

 /**
  * @file extension.cpp
  * @brief Implement extension code here.
  */

class RedisContextTypeHandler : public IHandleTypeDispatch
{
public:
    void OnHandleDestroy(HandleType_t type, void *object)
    {
        if (object) {
            delete (RedisConnect *)object;
        }
    }
};

class RedisReplyTypeHandler : public IHandleTypeDispatch
{
public:
    void OnHandleDestroy(HandleType_t type, void *object)
    {
        if (object) {
            delete (RedisReply *)object;
        }
    }
};

class AsyncRedisContextTypeHandler : public IHandleTypeDispatch
{
public:
    void OnHandleDestroy(HandleType_t type, void *object)
    {
        if (object) {
            delete (SMASyncRedis *)object;
        }
    }
};

HandleType_t g_RedisCtxType = 0;
RedisContextTypeHandler g_RedisCtxTypeHandler;

HandleType_t g_RedisReplyType = 0;
RedisReplyTypeHandler g_RedisReplyTypeHandler;

HandleType_t g_AsyncRedisCtxType = 0;
AsyncRedisContextTypeHandler g_AsyncRedisCtxTypeHandler;

HiredisExt g_HiredisExt;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_HiredisExt);

static void FrameHook(bool simulating)
{
    g_HiredisExt.RunFrame();
}

bool HiredisExt::SDK_OnLoad(char * error, size_t maxlen, bool late)
{
    sharesys->AddNatives(myself, g_RedisNatives);
    sharesys->RegisterLibrary(myself, "hiredis");

    g_RedisCtxType = handlesys->CreateType("RedisContext", &g_RedisCtxTypeHandler, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);
    g_RedisReplyType = handlesys->CreateType("RedisReply", &g_RedisReplyTypeHandler, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);
    g_AsyncRedisCtxType = handlesys->CreateType("AsyncRedisContext", &g_AsyncRedisCtxTypeHandler, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);
    return true;
}

void HiredisExt::SDK_OnAllLoaded()
{
    smutils->AddGameFrameHook(FrameHook);
}

void HiredisExt::SDK_OnUnload()
{
    smutils->RemoveGameFrameHook(FrameHook);

    handlesys->RemoveType(g_RedisCtxType, myself->GetIdentity());
    handlesys->RemoveType(g_RedisReplyType, myself->GetIdentity());
    handlesys->RemoveType(g_AsyncRedisCtxType, myself->GetIdentity());
}

//void HiredisExt::AddCallback(IPluginFunction * callback, std::vector<std::tuple<DataType, cell_t>> params)
//{
//    if (callback) {
//        cblist.enqueue({ callback, params });
//    }
//}

void HiredisExt::RequestFrame(std::function<void(void * data)> cb, void *data)
{
    cblist.enqueue({ cb, data });
}

void HiredisExt::RunFrame()
{
    if (cblist.size_approx()) {
        std::vector<CallbackInfo> list(cblist.size_approx());
        int n = cblist.try_dequeue_bulk(list.begin(), list.size());
        for (int i = 0; i < n; ++i) {
            auto[cb, data] = list[i];
            if (cb) {
                cb(data);
            }
        }
    }
}

IdentityToken_t *HiredisExt::GetIdentity() const
{
    return myself->GetIdentity();
}
