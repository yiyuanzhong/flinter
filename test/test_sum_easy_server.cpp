#include <stdlib.h>
#include <string.h>

#include <flinter/linkage/easy_context.h>
#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>

#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/object.h>
#include <flinter/signals.h>

static flinter::EasyServer *g_server;

class I : public flinter::Object {
public:
    I() : _i(0) {}
    int Add(int i)
    {
        _i += i;
        return _i;
    }

private:
    int _i;

}; // class Add

class Handler : public flinter::EasyHandler {
public:
    explicit Handler(const std::string &id) : _id(id) {}
    virtual ~Handler() {}
    virtual ssize_t GetMessageLength(const flinter::EasyContext &context,
                                     const void *buffer,
                                     size_t length)
    {
        const char *buf = reinterpret_cast<const char *>(buffer);

        printf("LEN ");
        for (size_t i = 0; i < length; ++i) {
            printf("%02X ", buf[i]);
        }
        printf("\n");

        size_t s = 1;
        for (const char *p = reinterpret_cast<const char *>(buffer);
             s <= length; ++p, ++s) {

            if (*p == '\n') {
                return s;
            } else if (*p == '\r') {
                if (s == length) {
                    return 1;
                }

                if (*(p + 1) == '\n') {
                    return s + 1;
                }

                return -1;
            }
        }

        return 0;
    }

    virtual int OnMessage(const flinter::EasyContext &context,
                          const void *buffer, size_t length)
    {
        const char *buf = reinterpret_cast<const char *>(buffer);

        printf("MSG ");
        for (size_t i = 0; i < length; ++i) {
            printf("%02X ", buf[i]);
        }
        printf("\n");

        int n = atoi(buf);
        if (n == 0) {
            if (!g_server->Send(context, "QUIT\r\n", 6)) {
                return -1;
            }

            g_server->Disconnect(context);
            return 1;
        }

        I *i = static_cast<I *>(context.context());
        int total = i->Add(n);
        std::ostringstream s;
        s << total << "\n";
        std::string str = s.str();
        if (!g_server->Send(context, str.data(), str.length())) {
            return -1;
        }

        return 1;
    }

    virtual bool OnConnected(const flinter::EasyContext &context)
    {
        context.set_context(new I);
        return true;
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

    flinter::EasyServer::Configure *c = s.configure();
    c->maximum_incoming_connections = 10;

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
