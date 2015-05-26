#include <stdlib.h>
#include <string.h>

#include <flinter/linkage/easy_context.h>
#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>

#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/signals.h>

static flinter::EasyServer *g_server;

class Handler : public flinter::EasyHandler {
public:
    explicit Handler(const std::string &id) : _id(id) {}
    virtual ~Handler() {}
    virtual ssize_t GetMessageLength(const flinter::EasyContext &context,
                                     const void *buffer,
                                     size_t length)
    {
        LOG(INFO) << "Handler: " << _id << " GetMessageLength(" << context.channel() << ")";
        return length;
    }

    virtual int OnMessage(const flinter::EasyContext &context,
                          const void *buffer, size_t length)
    {
        LOG(INFO) << "Handler: " << _id << " OnMessage(" << context.channel() << ")";
        if (memcmp(buffer, "quit\r\n", 6) == 0) {
            g_server->Send(context, "OK, goodbye!\r\n", 14);
            g_server->Disconnect(context, true);
        }

        g_server->Send(context.channel(), buffer, length);
        return 1;
    }

private:
    std::string _id;

}; // class Handler

int main()
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    signals_ignore(SIGPIPE);

    Handler handler1("1");
    Handler handler2("2");
    flinter::EasyServer s(&handler1);
    g_server = &s;

    if (!s.Listen(5566)) {
        return EXIT_FAILURE;
    }

    if (!s.Listen(5567, &handler2)) {
        return EXIT_FAILURE;
    }

    if (!s.Initialize(3, 7)) {
        return EXIT_FAILURE;
    }

    msleep(10000);
    s.Shutdown();
    return EXIT_SUCCESS;
}
