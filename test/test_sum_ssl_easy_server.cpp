#include <gtest/gtest.h>

#include <stdlib.h>
#include <string.h>

#include <flinter/linkage/easy_context.h>
#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>
#include <flinter/linkage/ssl_context.h>
#include <flinter/linkage/ssl_peer.h>

#include <flinter/thread/mutex.h>
#include <flinter/thread/mutex_locker.h>

#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/openssl.h>
#include <flinter/signals.h>

class Handler : public flinter::EasyHandler {
public:
    explicit Handler(const std::string &id) : _id(id), _i(0)
    {
        LOG(INFO) << "Handler::Handler(" << _id << "): " << this;
    }

    virtual ~Handler()
    {
        LOG(INFO) << "Handler::~Handler(" << _id << "): " << this;
    }

    virtual ssize_t GetMessageLength(const flinter::EasyContext & /*context*/,
                                     const void *buffer,
                                     size_t length)
    {
        const unsigned char *buf = reinterpret_cast<const unsigned char *>(buffer);

        printf("LEN ");
        for (size_t i = 0; i < length; ++i) {
            printf("%02X ", buf[i]);
        }
        printf("\n");

        size_t s = 1;
        for (const char *p = reinterpret_cast<const char *>(buffer);
             s <= length; ++p, ++s) {

            if (*p == '\n') {
                return static_cast<ssize_t>(s);
            } else if (*p == '\r') {
                if (s == length) {
                    return 0;
                }

                if (*(p + 1) == '\n') {
                    return static_cast<ssize_t>(s + 1);
                }

                return -1;
            }
        }

        return 0;
    }

    virtual int OnMessage(const flinter::EasyContext &context,
                          const void *buffer, size_t length)
    {
        const unsigned char *buf = reinterpret_cast<const unsigned char *>(buffer);
        flinter::EasyServer *server = context.easy_server();

        printf("MSG ");
        for (size_t i = 0; i < length; ++i) {
            printf("%02X ", buf[i]);
        }
        printf("\n");

        int n = atoi(reinterpret_cast<const char *>(buf));
        if (n == 0) {
            if (!server->Send(context, "QUIT\r\n", 6)) {
                return -1;
            }

            server->Disconnect(context);
            return 1;
        }

        int total = Add(n);

        std::ostringstream s;
        s << "Sum: " << total << "\n";
        std::string str = s.str();
        if (!server->Send(context, str.data(), str.length())) {
            return -1;
        }

        return 1;
    }

    virtual bool OnConnected(const flinter::EasyContext &context)
    {
        const flinter::SslPeer *peer = context.ssl_peer();
        LOG(INFO) << "SUBJECT: " << peer->subject_name();
        LOG(INFO) << "ISSUER: " << peer->issuer_name();
        LOG(INFO) << "SERIAL: " << peer->serial_number();
        return flinter::EasyHandler::OnConnected(context);
    }

    int Add(int n)
    {
        // If all connections share a single Handler, we lock.
        //
        // If each connection has its own Handler, we still lock, because two
        // job threads might process two messages belonging to one connection
        // simutaneously.
        //
        flinter::MutexLocker locker(&_mutex);
        _i += n;
        return _i;
    }

private:
    flinter::Mutex _mutex;
    std::string _id;
    int _i;

}; // class Handler

class Factory : public flinter::Factory<flinter::EasyHandler> {
public:
    explicit Factory(const std::string &id) : _id(id) {}
    virtual ~Factory() {}

    virtual flinter::EasyHandler *Create() const
    {
        return new Handler(_id);
    }

private:
    std::string _id;

}; // class Factory

static volatile sig_atomic_t g_quit = 0;

static void on_signal_quit(int /*signum*/)
{
    g_quit = 1;
}

TEST(sumServer, TestSslListen)
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    signals_set_handler(SIGQUIT, on_signal_quit);
    signals_set_handler(SIGTERM, on_signal_quit);
    signals_set_handler(SIGHUP, on_signal_quit);
    signals_set_handler(SIGINT, on_signal_quit);
    signals_ignore(SIGPIPE);

    flinter::OpenSSLInitializer openssl_initializer;

    flinter::SslContext *ssl = new flinter::SslContext(false);
    ASSERT_TRUE(ssl->SetVerifyPeer(true));
    ASSERT_TRUE(ssl->AddTrustedCACertificate("ssl.pem"));
    ASSERT_TRUE(ssl->LoadCertificate("ssl.pem"));
    ASSERT_TRUE(ssl->LoadPrivateKey("ssl.key","1234"));
    ASSERT_TRUE(ssl->VerifyPrivateKey());
    ASSERT_TRUE(ssl->SetSessionIdContext("helloWORLD"));
    ASSERT_TRUE(ssl->SetSessionTimeout(60));

    Factory factory("1");
    Handler handler("2");
    flinter::EasyServer s;

    flinter::EasyServer::Configure *c = s.configure();
    c->incoming_receive_timeout = 2000000000LL;
    c->incoming_idle_timeout = 10000000000LL;
    c->maximum_incoming_connections = 10;

    // Factory mode, each connection has its own Handler.
    ASSERT_TRUE(s.SslListen(5566, ssl, &factory));

    // Handler mode, all connections share a single Handler.
    ASSERT_TRUE(s.SslListen(5567, ssl, &handler));

    ASSERT_TRUE(s.Initialize(3, 7));

    while (!g_quit) {
        msleep(1000);
    }

    s.Shutdown();

    delete ssl;
}
