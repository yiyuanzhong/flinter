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

#include <flinter/linkage/linkage_handler.h>
#include <flinter/linkage/linkage_peer.h>
#include <flinter/linkage/linkage_worker.h>
#include <flinter/linkage/ssl_linkage.h>
#include <flinter/logger.h>
#include <flinter/signals.h>

static flinter::LinkageWorker *g_worker;

class H : public flinter::LinkageHandler {
public:
    virtual ~H() {}
    virtual ssize_t GetMessageLength(flinter::Linkage *linkage,
                                     const void *buffer, size_t length)
    {
        return length;
    }

    virtual int OnMessage(flinter::Linkage *linkage,
                          const void *buffer, size_t length)
    {
        const char *buf = reinterpret_cast<const char *>(buffer);
        for (size_t i = 0; i < length; ++i) {
            printf("%c", buf[i]);
        }

        printf("\n");
        return 1;
    }

    virtual bool OnConnected(flinter::Linkage *linkage)
    {
        flinter::SslLinkage *ssl = static_cast<flinter::SslLinkage *>(linkage);
        LOG(INFO) << "SUBJECT: " << ssl->peer_subject_name();
        LOG(INFO) << "ISSUER: " << ssl->peer_issuer_name();
        LOG(INFO) << "SERIAL: " << std::hex << ssl->peer_serial_number();
        flinter::LinkageHandler::OnConnected(ssl);
        return linkage->Send("GET /\r\n", 7);
    }

    virtual void OnDisconnected(flinter::Linkage *linkage)
    {
        g_worker->Shutdown();
    }

}; // class H

static void on_signal_quit(int /*signum*/)
{
    g_worker->Shutdown();
}

TEST(sslClient, TestConnect)
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);

    signals_set_handler(SIGHUP,  on_signal_quit);
    signals_set_handler(SIGINT,  on_signal_quit);
    signals_set_handler(SIGQUIT, on_signal_quit);
    signals_set_handler(SIGTERM, on_signal_quit);

    ASSERT_TRUE(SSL_library_init());
    SSL_load_error_strings();

    flinter::SslContext *ssl = new flinter::SslContext(true);
    ASSERT_TRUE(ssl->SetVerifyPeer(true));
    ASSERT_TRUE(ssl->AddTrustedCACertificate("ssl.pem"));

    ASSERT_TRUE(ssl->LoadCertificate("ssl.pem"));
    ASSERT_TRUE(ssl->LoadPrivateKey("ssl.key","1234"));
    ASSERT_TRUE(ssl->VerifyPrivateKey());

    H handler;
    flinter::LinkageBase *l = flinter::SslLinkage::ConnectTcp4(&handler, "127.0.0.1", 5566, ssl);
    ASSERT_TRUE(l);

    flinter::LinkageWorker worker;
    ASSERT_TRUE(l->Attach(&worker));

    g_worker = &worker;
    ASSERT_TRUE(worker.Run());

    delete ssl;

    SSL_library_cleanup();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    ERR_free_strings();
}
