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

#ifndef FLINTER_LINKAGE_SSL_CONTEXT_H
#define FLINTER_LINKAGE_SSL_CONTEXT_H

#include <string>

struct ssl_ctx_st;

namespace flinter {

class SslContext {
public:
    explicit SslContext(bool enhanced_security = true);
    ~SslContext();

    bool VerifyPrivateKey();
    bool SetAllowTlsTicket(bool allow);
    bool SetSessionTimeout(int seconds);
    bool LoadDHParam(const std::string &filename);
    bool LoadCertificate(const std::string &filename);
    bool SetSessionIdContext(const std::string &context);
    bool SetVerifyPeer(bool verify, bool required = true);
    bool LoadCertificateChain(const std::string &filename);
    bool AddTrustedCACertificate(const std::string &filename);
    bool LoadPrivateKey(const std::string &filename, const std::string &passphrase);

    struct ssl_ctx_st *context()
    {
        return _context;
    }

private:
    static int PasswordCallback(char *buf, int size, int rwflag, void *userdata);
    bool Initialize();

    struct ssl_ctx_st *_context;
    bool _enhanced_security;

}; // class SslContext

} // namespace flinter

#endif // FLINTER_LINKAGE_SSL_CONTEXT_H
