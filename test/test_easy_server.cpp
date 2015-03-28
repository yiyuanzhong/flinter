#include <stdlib.h>
#include <string.h>

#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>

#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/signals.h>

static flinter::EasyServer *g_server;

class Handler : public flinter::EasyHandler {
public:
    virtual ~Handler() {}
    virtual ssize_t GetMessageLength(uint64_t flow,
                                     const void *buffer,
                                     size_t length)
    {
        LOG(INFO) << "Handler: GetMessageLength(" << flow << ")";
        return length;
    }

    virtual int OnMessage(uint64_t flow, const void *buffer, size_t length)
    {
        LOG(INFO) << "Handler: OnMessage(" << flow << ")";
        if (memcmp(buffer, "quit\r\n", 6) == 0) {
            g_server->Send(flow, "OK, goodbye!\r\n", 14);
            g_server->Disconnect(flow, true);
        }

        g_server->Send(flow, buffer, length);
        return 1;
    }

}; // class Handler

int main()
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    signals_ignore(SIGPIPE);

    Handler handler;
    flinter::EasyServer s(&handler);
    g_server = &s;

    if (!s.Listen(5566)) {
        return EXIT_FAILURE;
    }

    if (!s.Initialize(3, 7)) {
        return EXIT_FAILURE;
    }

    msleep(10000);
    s.Shutdown();
    return EXIT_SUCCESS;
}
