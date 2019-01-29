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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

 /**
  * @file extension.h
  * @brief Sample extension code header.
  */

#include "SMASyncRedis.h"
#include "concurrentqueue.h"

#include <hiredis/hiredis.h>

#include "smsdk_ext.h"

  /**
   * @brief Sample implementation of the SDK Extension.
   * Note: Uncomment one of the pre-defined virtual functions in order to use it.
   */
class HiredisExt : public SDKExtension
{
public:
    /**
     * @brief This is called after the initial loading sequence has been processed.
     *
     * @param error		Error message buffer.
     * @param maxlen	Size of error message buffer.
     * @param late		Whether or not the module was loaded after map load.
     * @return			True to succeed loading, false to fail.
     */
    virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);

    /**
     * @brief This is called right before the extension is unloaded.
     */
    virtual void SDK_OnUnload();

    /**
    * @brief This is called once all known extensions have been loaded.
    * Note: It is is a good idea to add natives here, if any are provided.
    */
    virtual void SDK_OnAllLoaded();

    /**
    * @brief Called when the pause state is changed.
    */
    //virtual void SDK_OnPauseChange(bool paused);

    /**
    * @brief this is called when Core wants to know if your extension is working.
    *
    * @param error		Error message buffer.
    * @param maxlen	Size of error message buffer.
    * @return			True if working, false otherwise.
    */
    //virtual bool QueryRunning(char *error, size_t maxlen);

    void RequestFrame(std::function<void(void *data)> cb, void *data);

    void RunFrame();
    IdentityToken_t *GetIdentity() const;

public:
#if defined SMEXT_CONF_METAMOD
    /**
     * @brief Called when Metamod is attached, before the extension version is called.
     *
     * @param error			Error buffer.
     * @param maxlen		Maximum size of error buffer.
     * @param late			Whether or not Metamod considers this a late load.
     * @return				True to succeed, false to fail.
     */
     //virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);

     /**
      * @brief Called when Metamod is detaching, after the extension version is called.
      * NOTE: By default this is blocked unless sent from SourceMod.
      *
      * @param error			Error buffer.
      * @param maxlen		Maximum size of error buffer.
      * @return				True to succeed, false to fail.
      */
      //virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);

      /**
       * @brief Called when Metamod's pause state is changing.
       * NOTE: By default this is blocked unless sent from SourceMod.
       *
       * @param paused		Pause state being set.
       * @param error			Error buffer.
       * @param maxlen		Maximum size of error buffer.
       * @return				True to succeed, false to fail.
       */
       //virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif

private:
    using CallbackInfo = std::tuple<std::function<void(void *data)>, void *>;
    moodycamel::ConcurrentQueue<CallbackInfo> cblist;
};

extern HiredisExt g_HiredisExt;

extern sp_nativeinfo_t g_RedisNatives[];

extern HandleType_t g_RedisCtxType;
extern HandleType_t g_RedisReplyType;
extern HandleType_t g_AsyncRedisCtxType;

struct RedisConnect
{
    ~RedisConnect()
    {
        if (ctx) {
            redisFree(ctx);
        }
    }

    redisContext *ctx;
};

struct RedisReply
{
    ~RedisReply()
    {
        if (reply && flag) {
            freeReplyObject(reply);
        }
    }

    bool flag = true;
    redisReply *reply;
};

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
