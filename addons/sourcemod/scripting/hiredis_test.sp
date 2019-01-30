#include <sourcemod>

#include <hiredis>

RedisAsync async;

public void OnPluginStart()
{
    RegConsoleCmd("sm_hiredis_test", Cmd_HiredisTest);
    RegConsoleCmd("sm_asynchiredis_test", Cmd_ASyncHiredisTest);
    RegConsoleCmd("sm_asynchiredis_delete", Cmd_ASyncHiredisDel);
}

public void ConnectCallback(RedisAsync ac, int status, any data)
{
    PrintToServer("ConnectCallback: %d %d", status, data);

    // you can send "auth" right after calling connect
    ac.Command("auth foobared233");

    for (int i = 0; i < 10; ++i) {
        ac.Command("lpush alsit %d", i);
    }

    ac.Command("set foo %s", "wtf");
    ac.CommandEx(CommandCallback, 0, "get foo");
    ac.CommandEx(CommandCallback, 0, "lrange alist 0 -1");
    ac.Command("quit");
}

public void DisconnectCallback(RedisAsync ac, int status, any data)
{
    PrintToServer("DisconnectCallback: %d %d", status, data);
}

public void CommandCallback(RedisAsync ac, RedisReply reply, any data)
{
    // This will print only one reply if 
    PrintReply(reply);
}

public Action Cmd_ASyncHiredisDel(int client, int args)
{
    delete async;
    return Plugin_Handled;
}

public Action Cmd_ASyncHiredisTest(int client, int args)
{
    async = new RedisAsync();
    async.SetConnectCallback(ConnectCallback, 222);
    async.SetDisconnectCallback(DisconnectCallback, 333);

    if (!async.Connect("127.0.0.1", 6379)) {
        char buf[256];
        async.GetErrorString(buf, sizeof(buf));
        PrintToServer("RedisAsync Connect failed: %d, %s", async.GetError(), buf);
        return Plugin_Handled;
    }

    RedisAsync async2 = new RedisAsync();
    async2.SetConnectCallback(ConnectCallback, 444);
    async2.SetDisconnectCallback(DisconnectCallback, 666);

    if (!async2.Connect("127.0.0.1", 6379)) {
        char buf[256];
        async2.GetErrorString(buf, sizeof(buf));
        PrintToServer("RedisAsync 2 Connect failed: %d, %s", async2.GetError(), buf);
        return Plugin_Handled;
    }
    delete async2; // there will be no call back for async2

    return Plugin_Handled;
}

public Action Cmd_HiredisTest(int client, int args)
{
    Redis redis = new Redis();
    if (!redis.Connect("127.0.0.1", 6379)) {
        ThrowError("Cannot connect to redis server.");
    }

    RedisReply reply = redis.Command("AUTH foobared233");
    if (reply == INVALID_HANDLE) {
        ThrowError("Replay is invalid.")
    }
    PrintReply(reply);
    delete reply;

    for (int i = 0; i < 10; ++i) {
        redis.AppendCommand("lpush alist %d", i);
    }

    for (int i = 0; i < 10; ++i) {
        reply = redis.GetReply();
        if (reply == INVALID_HANDLE) {
            ThrowError("Replay is invalid.")
        }
        PrintReply(reply);
        delete reply;
    }

    redis.AppendCommand("set foo %s", "23333333");
    reply = redis.GetReply();
    if (reply == INVALID_HANDLE) {
        ThrowError("Replay is invalid.")
    }
    PrintReply(reply);
    delete reply;
    
    reply = redis.Command("get %s", "foo");
    if (reply == INVALID_HANDLE) {
        ThrowError("Replay is invalid.")
    }
    PrintReply(reply);
    delete reply;

    reply = redis.Command("lrange alist 0 -1");
    if (reply == INVALID_HANDLE) {
        ThrowError("Replay is invalid.")
    }
    PrintReply(reply);
    delete reply;

    delete redis;
    return Plugin_Handled;
}

stock void PrintReply(RedisReply reply)
{
    char buf[256];
    reply.GetString(buf, sizeof(buf));
    int count = reply.ArraySize;
    PrintToServer("Reply Type: %d, int: %d, arraysize: %d, string: %s", reply.Type, reply.IntValue, count, buf);
    for (int i = 0; i < count; ++i) {
        RedisReply element = reply.GetElement(i);
        if (element) {
            PrintReply(element);
        }
    }
}