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

#ifndef __FLINTER_LINKAGE_SSL_LINKAGE_H__
#define __FLINTER_LINKAGE_SSL_LINKAGE_H__

#include <stdint.h>

#include <flinter/linkage/linkage.h>
#include <flinter/linkage/ssl_context.h>

struct ssl_st;

namespace flinter {

class SslLinkage : public Linkage {
public:
    static SslLinkage *ConnectTcp4(LinkageHandler *handler,
                                   const std::string &host,
                                   uint16_t port,
                                   SslContext *context);

    static SslLinkage *ConnectUnix(LinkageHandler *handler,
                                   const std::string &sockname,
                                   bool file_based,
                                   SslContext *context);

    // Accepted.
    // @param handler life span NOT taken.
    // @param context life span NOT taken.
    SslLinkage(LinkageHandler *handler,
               const LinkagePeer &peer,
               const LinkagePeer &me,
               SslContext *context);

    virtual ~SslLinkage();

    virtual bool Send(const void *buffer, size_t length);

    bool IsValid() const
    {
        return _ssl != NULL;
    }

    const std::string &peer_subject_name() const
    {
        return _peer_subject_name;
    }

    const std::string &peer_issuer_name() const
    {
        return _peer_issuer_name;
    }

    uint64_t peer_serial_number() const
    {
        return _peer_serial_number;
    }

protected:
    SslLinkage(LinkageHandler *handler,
               const std::string &name,
               SslContext *context);

    virtual int OnReadable(LinkageWorker *worker);
    virtual int OnWritable(LinkageWorker *worker);
    virtual bool OnConnected();
    virtual int Shutdown();

private:
    enum Action {
        kActionNone,
        kActionRead,
        kActionWrite,
        kActionAccept,
        kActionConnect,
        kActionShutdown,
    }; // enum Action

    static const char *GetActionString(const Action in_progress);

    int HandleError(LinkageWorker *worker,
                    const Action in_progress, int ret);

    int OnEvent(LinkageWorker *worker, bool reading_or_writing);
    int DoShutdown(LinkageWorker *worker);
    int DoConnect(LinkageWorker *worker);
    bool GetInformation();

    struct ssl_st *_ssl;
    Action _in_progress;
    bool _shutdown;

    std::string _peer_subject_name;
    std::string _peer_issuer_name;
    uint64_t _peer_serial_number;

}; // class SslLinkage

} // namespace flinter

#endif // __FLINTER_LINKAGE_SSL_LINKAGE_H__
