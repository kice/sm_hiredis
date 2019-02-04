#if defined _hiredis_included
 #endinput
#endif
#define _hiredis_included

// A warpper of Hiredis for Sourcemod

#define REDIS_ERR -1
#define REDIS_OK 0

/* When an error occurs, the err flag in a context is set to hold the type of
 * error that occured. REDIS_ERR_IO means there was an I/O error and you
 * should use the "errno" variable to find out what is wrong.
 * For other values, the "errstr" field will hold a description. */
#define REDIS_ERR_IO 1 /* Error in read or write */
#define REDIS_ERR_EOF 3 /* End of file */
#define REDIS_ERR_PROTOCOL 4 /* Protocol error */
#define REDIS_ERR_OOM 5 /* Out of memory */
#define REDIS_ERR_OTHER 2 /* Everything else... */

#define REDIS_REPLY_STRING 1
#define REDIS_REPLY_ARRAY 2
#define REDIS_REPLY_INTEGER 3
#define REDIS_REPLY_NIL 4
#define REDIS_REPLY_STATUS 5
#define REDIS_REPLY_ERROR 6

methodmap RedisReply < Handle
{
    /**
     * return REDIS_REPLY_*
     */
    property int Type {
        public native get();
    }

    /**
     * The integer when type is REDIS_REPLY_INTEGER
     */
    property int IntValue {
        public native get();
    }

    /**
     * number of elements, for REDIS_REPLY_ARRAY
     */
    property int ArraySize {
        public native get();
    }

    /**
     * Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING
     */
    public native int GetString(char[] buf, int maxlen);

    /**
     * Get an element from the vector for REDIS_REPLY_ARRAY. 
     * You need to call `CloseHandle()` or use `delete` to free all element that had retrieved.
     * The rest which did not touch, no need to get it and free it.
     */
    public native RedisReply GetElement(int index);
}

methodmap Redis < Handle
{
    /**
     * Create a new object for blocking hiredis API
     */
    public native Redis();

    /**
     * Setup an connection to the server. ALL REDIS OBJECTS CANNOT BE REUSE.
     * You need to close the handle and create a new if you encounter any error.
     */
    public native bool Connect(const char[] ip, int port);

    /**
     * Same as `Connect`, but with timeout feature.
     */
    public native bool ConnectWithTimeout(const char[] ip, int port, int timeout);

    /**
     * This function first checks if there are unconsumed
     * replies to return and returns one if so. Otherwise, it flushes the output
     * buffer to the socket and reads until it has a reply.
     * If there is no reply in the queue, it will block the thread.
     */
    public native RedisReply GetReply();

    /**
     * Write a command to the output buffer. Use this functions to get a pipeline of commands.
     */
    public native bool AppendCommand(const char[] format, any ...);

    /**
     * It is identical to calling `AppendCommand()`, followed by `GetReply()`.
     */
    public native RedisReply Command(const char[] format, any ...);

    /**
     * Error flags, 0 when there is no error
     */
    property int Error {
        public native get();
    }

    /**
     * String representation of error when applicable
     */
    public native int GetErrorString(char[] buf, int maxlen);

    public native bool SetTimeout(int timeout);

    public native bool EnableKeepAlive();
}

typedef RedisAsyncCommandCallback = function void(RedisAsync ac, RedisReply reply, any data);

methodmap RedisAsync < Handle
{
    /**
     * Create a new redis client
     *
     * @param piped_cache   The number of command put to pipeline before commit
     *                      0: manully commit, 1: disable pipeline (commit when available)
     *
     * @param pipeline_timeout  The maximum time to spend waiting for pipeline to be filled in ms
     *                          0: wait for piped_cache number of command to be filled.
     *                          DONT recommend set time to 0, since the worker will wait forever
     *                          until receive enough command (if you use future you will get a deadlock)
     *
     */
    public native RedisAsync(int piped_cache = 0, int pipeline_timeout = 0);

    /**
     * Setup an connection to the server.
     */
    public native bool Connect(const char[] ip, int port = 6379, int timeout = 1500);

    property bool IsConnected {
        public native get();
    }

    // this will be called after deleting `RedisAsync`
    public native void Disconnect();

    /**
     * Send an asynchronously command to redis server, and callback with reply.
     */
    public native RedisAsync Append(RedisAsyncCommandCallback callback, any data, const char[] format, any ...);

    /**
     * Send an asynchronously command to redis server, but dont return until got the reply, like blocking-api.
     */
    public native RedisReply Command(const char[] format, any ...);

    // This function will force worker to commit existing command when pipeline was enabled
    public native RedisAsync Commit();

    property int Error {
        public native get();
    }

    public native int GetErrorString(char[] buf, int maxlen);
}

/**
 * Do not edit below this line!
 */
public Extension __ext_hiredis = 
{
	name = "Hiredis",
	file = "hiredis.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

