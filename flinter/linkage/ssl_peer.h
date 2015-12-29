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

#ifndef __FLINTER_LINKAGE_SSL_PEER_H__
#define __FLINTER_LINKAGE_SSL_PEER_H__

#include <stdint.h>

#include <string>

namespace flinter {

class SslPeer {
public:
    SslPeer(const std::string &subject_name,
            const std::string &issuer_name,
            uint64_t serial_number)
            : _subject_name(subject_name)
            , _issuer_name(issuer_name)
            , _serial_number(serial_number) {}

    const std::string &subject_name() const
    {
        return _subject_name;
    }

    const std::string &issuer_name() const
    {
        return _issuer_name;
    }

    const uint64_t &serial_number() const
    {
        return _serial_number;
    }

private:
    std::string _subject_name;
    std::string _issuer_name;
    uint64_t _serial_number;

}; // class SslPeer

} // namespace flinter

#endif // __FLINTER_LINKAGE_SSL_PEER_H__
