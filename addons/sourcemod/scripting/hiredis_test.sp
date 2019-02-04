#include <sourcemod>

#include <hiredis>

RedisAsync async = null;

public void OnPluginStart()
{
    RegConsoleCmd("sm_hiredis_test", Cmd_HiredisTest);
    RegConsoleCmd("sm_asynchiredis_test", Cmd_ASyncHiredisTest);
    RegConsoleCmd("sm_asynchiredis_test2", Cmd_ASyncHiredisTest2);

    RegConsoleCmd("sm_asynchiredis_err", Cmd_ASyncHiredisErr);
    RegConsoleCmd("sm_asynchiredis_delete", Cmd_ASyncHiredisDel);
}

public void CommandCallback(RedisAsync ac, RedisReply reply, any data)
{
    PrintReply(reply);
}

public Action Cmd_ASyncHiredisDel(int client, int args)
{
    delete async;
    async = null;
    return Plugin_Handled;
}

public Action Cmd_ASyncHiredisTest(int client, int args)
{
    async = new RedisAsync();
    if (!async.Connect("127.0.0.1")) {
        char buf[256];
        async.GetErrorString(buf, sizeof(buf));
        PrintToServer("RedisAsync Connect failed: %d, %s", async.Error, buf);
        return Plugin_Handled;
    }
    async.Append(CommandCallback, 0, "AUTH foobared233");
    async.Commit();

    async.Append(CommandCallback, 0, "PING");
    async.Append(CommandCallback, 0, "SET foo 66666");
    async.Append(CommandCallback, 0, "GET foo");
    async.Append(CommandCallback, 0, "INCR counter");
    async.Commit();

    RedisAsync async2 = new RedisAsync();
    if (!async2.Connect("127.0.0.1")) {
        char buf[256];
        async2.GetErrorString(buf, sizeof(buf));
        PrintToServer("RedisAsync 2 Connect failed: %d, %s", async2.Error, buf);
        return Plugin_Handled;
    }
    delete async.Command("AUTH foobared233");

    async.Command("DEL mylist-sm");
    for (int i = 0; i < 10; ++i) {
        char element[16];
        Format(element, sizeof(element), "element-%d", i);
        async.Append(CommandCallback, 0, "LPUSH mylist-sm %s", element);
    }
    async.Commit();

    RedisReply reply = async.Command("LRANGE mylist-sm 0 -1");
    if (reply != null) {
        PrintReply(reply);
    }
    delete reply;

    delete async2; // there will be no call back for async2

    return Plugin_Handled;
}

public Action Cmd_ASyncHiredisTest2(int client, int args)
{
    if (async == null) {
        return Plugin_Continue;
    }

    char cmd[64] = "PING";
    if (args > 1) {
        GetCmdArg(1, cmd, sizeof(cmd));
    }

    ReplyToCommand(client, "Appending async command \"%s\" to redis server...", cmd);
    async.Append(CommandCallback, 0, cmd);
    async.Commit();
    return Plugin_Handled;
}

public Action Cmd_ASyncHiredisErr(int client, int args)
{
    if (async == null) {
        return Plugin_Continue;
    }

    char err[128];
    async.GetErrorString(err, sizeof(err));
    ReplyToCommand(client, "Error: %s (code: %d)", err, async.Error);
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