#include <stdlib.h>
#include <string.h>

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

    if (!s.Initialize(3, 7)) {
        return EXIT_FAILURE;
    }

    uint64_t channel = s.ConnectTcp4("127.0.0.1", 5577, &handler);
    if (!channel) {
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
