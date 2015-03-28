/* Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <flinter/linkage/listener.h>
#include <flinter/linkage/linkage_handler.h>
#include <flinter/linkage/linkage_peer.h>
#include <flinter/linkage/linkage_worker.h>
#include <flinter/linkage/ssl_linkage.h>
#include <flinter/logger.h>
#include <flinter/signals.h>

static flinter::LinkageHandler *g_handler;
static flinter::LinkageWorker *g_worker;
static flinter::SslContext *g_ssl;

class I : public flinter::SslLinkage {
public:
    I(flinter::LinkageHandler *h,
      const flinter::LinkagePeer &peer,
      const flinter::LinkagePeer &me,
      flinter::SslContext *ssl) : SslLinkage(h, peer, me, ssl), _s(0) {}

    virtual ~I() {}
    int Add(int n)
    {
        return (_s += n);
    }

private:
    int _s;

}; // class I

class H : public flinter::LinkageHandler {
public:
    virtual ~H() {}
    virtual ssize_t GetMessageLength(flinter::Linkage *linkage,
                                     const void *buffer, size_t length)
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

    virtual int OnMessage(flinter::Linkage *linkage,
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
            if (!linkage->Send("QUIT\r\n", 6)) {
                return -1;
            }

            linkage->Disconnect();
            return 1;
        }

        I *i = static_cast<I *>(linkage);
        int total = i->Add(n);
        std::ostringstream s;
        s << total << "\n";
        std::string str = s.str();
        if (!linkage->Send(str.data(), str.length())) {
            return -1;
        }

        return 1;
    }

    virtual bool OnConnected(flinter::Linkage *linkage)
    {
        flinter::SslLinkage *ssl = static_cast<flinter::SslLinkage *>(linkage);
        LOG(INFO) << "SUBJECT: " << ssl->peer_subject_name();
        LOG(INFO) << "ISSUER: " << ssl->peer_issuer_name();
        LOG(INFO) << "SERIAL: " << std::hex << ssl->peer_serial_number();
        return flinter::LinkageHandler::OnConnected(ssl);
    }

}; // class H

class L : public flinter::Listener {
public:
    virtual ~L() {}
protected:
    virtual flinter::LinkageBase *CreateLinkage(const flinter::LinkagePeer &peer,
                                                const flinter::LinkagePeer &me)
    {
        LOG(INFO) << "Accept: " << me.ip_str() << ":" << me.port()
                  << " <- " << peer.ip_str() << ":" << peer.port();

        flinter::Linkage *l = new I(g_handler, peer, me, g_ssl);
        l->set_receive_timeout(5000000000);
        return l;
    }

}; // class L

static void on_signal_quit(int /*signum*/)
{
    g_worker->Shutdown();
}

TEST(sumServer, TestListen)
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);

    signals_set_handler(SIGHUP,  on_signal_quit);
    signals_set_handler(SIGINT,  on_signal_quit);
    signals_set_handler(SIGQUIT, on_signal_quit);
    signals_set_handler(SIGTERM, on_signal_quit);

    ASSERT_TRUE(SSL_library_init());
    SSL_load_error_strings();

    g_ssl = new flinter::SslContext;
    ASSERT_TRUE(g_ssl->SetVerifyPeer(true));
    ASSERT_TRUE(g_ssl->AddTrustedCACertificate("ssl.pem"));
    ASSERT_TRUE(g_ssl->LoadCertificate("ssl.pem"));
    ASSERT_TRUE(g_ssl->LoadPrivateKey("ssl.key","1234"));
    ASSERT_TRUE(g_ssl->VerifyPrivateKey());

    H handler;
    g_handler = &handler;

    L listener;
    ASSERT_TRUE(listener.ListenTcp(5566, false));

    flinter::LinkageWorker worker;
    ASSERT_TRUE(listener.Attach(&worker));

    g_worker = &worker;
    ASSERT_TRUE(worker.Run());

    delete g_ssl;

    SSL_library_cleanup();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    ERR_free_strings();
}
