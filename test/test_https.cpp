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

#include <flinter/linkage/interface.h>
#include <flinter/linkage/listener.h>
#include <flinter/linkage/linkage.h>
#include <flinter/linkage/linkage_handler.h>
#include <flinter/linkage/linkage_peer.h>
#include <flinter/linkage/linkage_worker.h>
#include <flinter/linkage/ssl_context.h>
#include <flinter/linkage/ssl_io.h>
#include <flinter/linkage/ssl_peer.h>
#include <flinter/logger.h>
#include <flinter/signals.h>

static flinter::LinkageHandler *g_handler;
static flinter::LinkageWorker *g_worker;
static flinter::SslContext *g_ssl;

class I : public flinter::Linkage {
public:
    I(flinter::AbstractIo *io, flinter::LinkageHandler *h,
      const flinter::LinkagePeer &peer,
      const flinter::LinkagePeer &me)
            : Linkage(io, h, peer, me), _s(0) {}

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
    virtual ssize_t GetMessageLength(flinter::Linkage * /*linkage*/,
                                     const void * /*buffer*/, size_t length)
    {
        return static_cast<ssize_t>(length);
    }

    virtual int OnMessage(flinter::Linkage *linkage,
                          const void * /*buffer*/, size_t /*length*/)
    {
        char buf[1024];
        snprintf(buf, sizeof(buf),
                "HTTP/1.1 200 OK\r\n"
                "Server: Apache\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 14\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Hello World!\r\n");

        return linkage->Send(buf, strlen(buf)) ? 1 : -1;
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

        flinter::Interface *i = new flinter::Interface;
        i->Accepted(peer.fd());
        flinter::AbstractIo *io = new flinter::SslIo(i, false, false, g_ssl);
        flinter::Linkage *l = new I(io, g_handler, peer, me);
        l->set_receive_timeout(5000000000);
        l->set_connect_timeout(2000000000);
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

    g_ssl = new flinter::SslContext(false);
    ASSERT_TRUE(g_ssl->SetVerifyPeer(false));
    ASSERT_TRUE(g_ssl->AddTrustedCACertificate("ssl.pem"));
    ASSERT_TRUE(g_ssl->LoadDHParam("dh.pem"));
    ASSERT_TRUE(g_ssl->LoadCertificate("ssl.pem"));
    ASSERT_TRUE(g_ssl->LoadPrivateKey("ssl.key","1234"));
    ASSERT_TRUE(g_ssl->VerifyPrivateKey());

    H handler;
    g_handler = &handler;

    L listener;
    ASSERT_TRUE(listener.ListenTcp(8081, false));

    flinter::LinkageWorker worker;
    ASSERT_GT(listener.Attach(&worker), 0);

    g_worker = &worker;
    ASSERT_TRUE(worker.Run());

    delete g_ssl;

    SSL_library_cleanup();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    ERR_free_strings();
}
