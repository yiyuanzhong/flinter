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

#include "flinter/linkage/ssl_context.h"

#include "config.h"
#if HAVE_OPENSSL_OPENSSLV_H
#include <openssl/ssl.h>
#include <openssl/ssl23.h>

namespace flinter {

SslContext::SslContext(bool enhanced_security)
        : _context(NULL)
        , _enhanced_security(enhanced_security)
{
    Initialize();
}

SslContext::~SslContext()
{
    if (_context) {
        SSL_CTX_free(_context);
    }
}

bool SslContext::Initialize()
{
    if (_context) {
        return true;
    }

    const SSL_METHOD *method;
    const char *ciphers = NULL;
    if (_enhanced_security) {
        ciphers = "TLSv1.2:!aNULL:!eNULL";
        method = TLSv1_2_method();
    } else {
        method = SSLv23_method();
    }

    _context = SSL_CTX_new(method);
    if (!_context) {
        return false;
    }

    if (ciphers) {
        if (SSL_CTX_set_cipher_list(_context, ciphers) != 1) {
            SSL_CTX_free(_context);
            _context = NULL;
            return false;
        }
    }

    SSL_CTX_set_default_passwd_cb(_context, PasswordCallback);
    SSL_CTX_set_default_passwd_cb_userdata(_context, NULL);
    return true;
}

bool SslContext::LoadCertificateChain(const std::string &filename)
{
    if (filename.empty()) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    if (!SSL_CTX_use_certificate_chain_file(_context, filename.c_str())) {
        return false;
    }

    return true;
}

bool SslContext::LoadCertificate(const std::string &filename)
{
    if (filename.empty()) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    if (!SSL_CTX_use_certificate_file(_context,
                                      filename.c_str(),
                                      SSL_FILETYPE_PEM)) {

        return false;
    }

    return true;
}

bool SslContext::LoadPrivateKey(const std::string &filename,
                                const std::string &passphrase)
{
    if (filename.empty()) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    SSL_CTX_set_default_passwd_cb_userdata(_context,
                                           const_cast<std::string *>(&passphrase));

    int ret = SSL_CTX_use_PrivateKey_file(_context,
                                          filename.c_str(),
                                          SSL_FILETYPE_PEM);

    SSL_CTX_set_default_passwd_cb_userdata(_context, NULL);
    if (!ret) {
        return false;
    }

    return true;
}

int SslContext::PasswordCallback(char *buf, int size, int rwflag, void *userdata)
{
    const std::string *s = reinterpret_cast<const std::string *>(userdata);
    if (!s) {
        return -1;
    }

    int ret = snprintf(buf, size, "%s", s->c_str());
    if (ret < 0 || ret >= size) {
        return -1;
    }

    return ret;
}

bool SslContext::VerifyPrivateKey()
{
    if (!Initialize()) {
        return false;
    }

    if (!SSL_CTX_check_private_key(_context)) {
        return false;
    }

    return true;
}

bool SslContext::SetVerifyPeer(bool verify)
{
    if (!Initialize()) {
        return false;
    }

    int mode = verify ? SSL_VERIFY_FAIL_IF_NO_PEER_CERT
                      | SSL_VERIFY_CLIENT_ONCE
                      | SSL_VERIFY_PEER
                      : SSL_VERIFY_NONE;

    SSL_CTX_set_verify(_context, mode, NULL);
    return true;
}

bool SslContext::AddTrustedCACertificate(const std::string &filename)
{
    if (filename.empty()) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    BIO *bp = BIO_new_file(filename.c_str(), "rb");
    if (!bp) {
        return false;
    }

    X509 *x509 = PEM_read_bio_X509(bp, NULL, NULL, (void *)"");
    BIO_free(bp);

    if (!x509) {
        return false;
    }

    X509_STORE *store = SSL_CTX_get_cert_store(_context);
    if (!store) {
        X509_free(x509);
        return false;
    }

    if (X509_STORE_add_cert(store, x509) != 1       ||
        SSL_CTX_add_client_CA(_context, x509) != 1  ){

        X509_free(x509);
        return false;
    }

    X509_free(x509);
    return true;
}

} // namespace flinter

#endif // HAVE_OPENSSL_OPENSSLV_H
