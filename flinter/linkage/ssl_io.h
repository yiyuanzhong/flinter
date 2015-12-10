/* Copyright 2015 yiyuanzhong@gmail.com (Yiyuan Zhong)
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

#ifndef __FLINTER_LINKAGE_SSL_IO_H__
#define __FLINTER_LINKAGE_SSL_IO_H__

#include <string>

#include <flinter/linkage/abstract_io.h>

struct ssl_st;

namespace flinter {

class Interface;
class SslContext;

class SslIo : public AbstractIo {
public:
    /// @param i life span TAKEN.
    /// @param context life span NOT taken.
    SslIo(Interface *i, bool connecting, SslContext *context);
    virtual ~SslIo();

    virtual Status Write(const void *buffer, size_t length, size_t *retlen);
    virtual Status Read(void *buffer, size_t length, size_t *retlen);
    virtual Status Initialize(Action *action, Action *next_action);
    virtual Status Shutdown();
    virtual Status Connect();
    virtual Status Accept();

    const std::string &peer_subject_name() const
    {
        return _peer_subject_name;
    }

    const std::string &peer_issuer_name() const
    {
        return _peer_issuer_name;
    }

    const uint64_t &peer_serial_number() const
    {
        return _peer_serial_number;
    }

private:
    static const char *GetActionString(const Action &in_progress);
    bool OnHandshaked();

    Status HandleError(const Action &in_progress, int ret);
    Status OnEvent(bool reading_or_writing);
    Status DoShutdown();
    Status DoConnect();

    struct ssl_st *_ssl;
    bool _connecting;
    Interface *_i;

    std::string _peer_subject_name;
    std::string _peer_issuer_name;
    uint64_t _peer_serial_number;

}; // class SslIo

} // namespace flinter

#endif // __FLINTER_LINKAGE_SSL_IO_H__
