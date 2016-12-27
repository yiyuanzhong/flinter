#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include <flinter/linkage/easy_context.h>
#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>

#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/signals.h>

class Handler : public flinter::EasyHandler {
public:
    virtual ~Handler() {}
    virtual ssize_t GetMessageLength(const flinter::EasyContext &context,
                                     const void *buffer,
                                     size_t length)
    {
        LOG(INFO) << "Handler: GetMessageLength(" << context.channel() << ")";
        return length;
    }

    virtual int OnMessage(const flinter::EasyContext &context,
                          const void *buffer, size_t length)
    {
        LOG(INFO) << "Handler: OnMessage(" << context.channel() << ", "
                  << length << ")";

        return 1;
    }

}; // class Handler

int main()
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    signals_ignore(SIGPIPE);

    Handler handler;
    flinter::EasyServer s;

    if (!s.Initialize(2, 0)) {
        return EXIT_FAILURE;
    }

    flinter::EasyServer::ConnectOption o;
    o.connect_socket.domain = AF_UNIX;
    o.connect_socket.unix_pathname = "/tmp/test_easy_client_low.sock";
    o.easy_handler = &handler;

    uint64_t channel = s.Connect(o);
    if (!channel) {
        s.Shutdown();
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 120; ++i) {
        std::ostringstream o;
        o << i << "\r\n";
        const std::string &str = o.str();
        s.Send(channel, str.data(), str.length());
        msleep(1000);
    }

    s.Shutdown();
    return EXIT_SUCCESS;
}
