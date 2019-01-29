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

    ac.Command("auth foobared233");
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