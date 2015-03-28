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

#include "flinter/linkage/listener.h"
#include "flinter/linkage/linkage_handler.h"
#include "flinter/linkage/linkage_peer.h"
#include "flinter/linkage/linkage_worker.h"
#include "flinter/linkage/linkage.h"
#include "flinter/logger.h"
#include "flinter/signals.h"

static flinter::LinkageHandler *g_handler;
static flinter::LinkageWorker *g_worker;

class H : public flinter::LinkageHandler {
public:
    virtual ~H() {}
    virtual ssize_t GetMessageLength(flinter::Linkage *linkage,
                                     const void *buffer, size_t length)
    {
        return length;
    }

    virtual int OnMessage(flinter::Linkage *linkage, const void *buffer, size_t length)
    {
        linkage->Send(buffer, length);
        if (memcmp(buffer, "quit\r\n", 6) == 0) {
            if (!linkage->Send("QUIT\r\n", 6)) {
                return -1;
            }

            linkage->Disconnect();
            return 1;
        }

        return 1;
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

        flinter::Linkage *l = new flinter::Linkage(g_handler, peer, me);
        l->set_receive_timeout(15000000000);
        return l;
    }

}; // class L

static void on_signal_quit(int /*signum*/)
{
    g_worker->Shutdown();
}

TEST(echoServer, TestListen)
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);

    signals_set_handler(SIGHUP,  on_signal_quit);
    signals_set_handler(SIGINT,  on_signal_quit);
    signals_set_handler(SIGQUIT, on_signal_quit);
    signals_set_handler(SIGTERM, on_signal_quit);

    H handler;
    g_handler = &handler;

    L listener;
    ASSERT_TRUE(listener.ListenTcp(5566, false));

    flinter::LinkageWorker worker;
    ASSERT_TRUE(listener.Attach(&worker));

    g_worker = &worker;
    ASSERT_TRUE(worker.Run());
}
