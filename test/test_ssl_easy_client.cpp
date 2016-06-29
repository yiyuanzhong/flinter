#include <gtest/gtest.h>

#include <stdlib.h>
#include <string.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <flinter/linkage/easy_context.h>
#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>
#include <flinter/linkage/ssl_context.h>
#include <flinter/linkage/ssl_peer.h>

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

    virtual bool OnConnected(const flinter::EasyContext &context)
    {
        const flinter::SslPeer *peer = context.ssl_peer();
        LOG(INFO) << "SUBJECT: " << peer->subject_name();
        LOG(INFO) << "ISSUER: " << peer->issuer_name();
        LOG(INFO) << "SERIAL: " << peer->serial_number();
        return flinter::EasyHandler::OnConnected(context);
    }

}; // class Handler

TEST(easyClient, TestSslConnect4)
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    signals_ignore(SIGPIPE);

    ASSERT_TRUE(SSL_library_init());
    SSL_load_error_strings();

    flinter::SslContext *ssl = new flinter::SslContext(false);
    ASSERT_TRUE(ssl->SetVerifyPeer(true));
    ASSERT_TRUE(ssl->AddTrustedCACertificate("ssl.pem"));
    ASSERT_TRUE(ssl->LoadCertificate("ssl.pem"));
    ASSERT_TRUE(ssl->LoadPrivateKey("ssl.key","1234"));
    ASSERT_TRUE(ssl->VerifyPrivateKey());

    Handler handler;
    flinter::EasyServer s;
    ASSERT_TRUE(s.Initialize(3, 7));

    uint64_t channel = s.SslConnectTcp4("127.0.0.1", 5577, ssl, &handler);
    ASSERT_NE(channel, 0);

    for (int i = 0; i < 120; ++i) {
        std::ostringstream o;
        o << i << "\r\n";
        const std::string &str = o.str();
        s.Send(channel, str.data(), str.length());
        msleep(1000);
    }

    s.Shutdown();
    delete ssl;
    SSL_library_cleanup();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    ERR_free_strings();
}
