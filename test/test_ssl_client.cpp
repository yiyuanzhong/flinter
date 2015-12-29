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
#include <flinter/linkage/linkage_handler.h>
#include <flinter/linkage/linkage_peer.h>
#include <flinter/linkage/linkage_worker.h>
#include <flinter/linkage/linkage.h>
#include <flinter/linkage/ssl_context.h>
#include <flinter/linkage/ssl_io.h>
#include <flinter/linkage/ssl_peer.h>
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
        linkage->Send(buffer, length);
        if (memcmp(buffer, "exit\r\n", 6) == 0 ||
            memcmp(buffer, "exit\n", 5) == 0   ){

            if (!linkage->Send("QUIT\r\n", 6)) {
                return -1;
            }

            return linkage->Disconnect();
        }

        return 1;
    }

    virtual bool OnConnected(flinter::Linkage *linkage)
    {
        flinter::SslIo *ssl = static_cast<flinter::SslIo *>(linkage->io());
        const flinter::SslPeer *peer = ssl->peer();
        LOG(INFO) << "SUBJECT: " << peer->subject_name();
        LOG(INFO) << "ISSUER: " << peer->issuer_name();
        LOG(INFO) << "SERIAL: " << std::hex << peer->serial_number();
        flinter::LinkageHandler::OnConnected(linkage);
        return linkage->Send("12345\r\n", 7);
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
    flinter::LinkagePeer me;
    flinter::LinkagePeer peer;
    flinter::Interface *i = new flinter::Interface;
    ASSERT_TRUE(i->ConnectTcp4("127.0.0.1", 5566, &peer, &me));

    flinter::SslIo *io = new flinter::SslIo(i, true, ssl);
    flinter::Linkage *l = new flinter::Linkage(io, &handler, peer, me);

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
