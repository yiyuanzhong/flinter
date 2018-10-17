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

#include <string.h>

#include <vector>

#include "flinter/logger.h"

#include "config.h"
#if HAVE_OPENSSL_OPENSSLV_H
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

namespace flinter {

int SslContext::_tls_ticket_key_index = -1;
typedef struct {
    unsigned char hmac[32];
    unsigned char ekey[32];
    unsigned char name[16];
    size_t bits;
} key_t;

SslContext::SslContext(bool enhanced_security)
        : _context(NULL)
        , _enhanced_security(enhanced_security)
{
    Initialize();
}

SslContext::~SslContext()
{
    if (!_context) {
        return;
    }

    std::vector<key_t> *const keys =
            reinterpret_cast<std::vector<key_t> *>(
                    SSL_CTX_get_ex_data(_context, _tls_ticket_key_index));

    if (keys) {
        delete keys;
        SSL_CTX_set_ex_data(_context, _tls_ticket_key_index, NULL);
    }

    SSL_CTX_free(_context);
}

bool SslContext::Initialize()
{
    static const unsigned char kDefaultDHParam[] = {
        0x30, 0x82, 0x01, 0x08, 0x02, 0x82, 0x01, 0x01, 0x00, 0xd8, 0x7f, 0x7c,
        0xe6, 0xbd, 0x7a, 0xd0, 0x30, 0x84, 0xda, 0x77, 0x5b, 0x44, 0xb2, 0xa0,
        0xde, 0x01, 0x26, 0x9f, 0xb7, 0xc9, 0xf3, 0xbf, 0x33, 0x5b, 0x36, 0x47,
        0x66, 0xc1, 0xcc, 0x4c, 0x53, 0x4b, 0x20, 0xc2, 0x57, 0xbc, 0x5b, 0x7e,
        0x10, 0x56, 0x77, 0xf3, 0xff, 0x6a, 0x19, 0x0d, 0x36, 0xe0, 0x12, 0xe7,
        0x77, 0x5e, 0x6a, 0xfe, 0x7b, 0x0a, 0x9f, 0x80, 0x7f, 0xe3, 0xc0, 0xc8,
        0xee, 0x08, 0x5d, 0x5a, 0xc9, 0x61, 0xfc, 0x1c, 0x08, 0x81, 0xdf, 0x4d,
        0x10, 0xa1, 0x8c, 0x7d, 0x21, 0xed, 0x48, 0x83, 0xa0, 0x25, 0x07, 0x66,
        0x24, 0x07, 0x17, 0x6f, 0x62, 0xab, 0x98, 0x67, 0x27, 0xd8, 0xb9, 0x54,
        0x2e, 0x1e, 0x6b, 0xa7, 0x44, 0x31, 0xae, 0x8d, 0x5c, 0xed, 0x0b, 0xcc,
        0xf2, 0x6b, 0x2f, 0xa2, 0xf4, 0x07, 0x38, 0x3f, 0x37, 0x37, 0xab, 0x2e,
        0x06, 0xcb, 0x6b, 0x79, 0xff, 0xf2, 0x98, 0xf2, 0x80, 0xe3, 0x31, 0xa7,
        0x5d, 0x82, 0xd1, 0xf9, 0xa8, 0x1a, 0x05, 0x02, 0x34, 0x3d, 0x59, 0xb6,
        0x28, 0x53, 0xa4, 0xb9, 0xef, 0xd8, 0xca, 0xa3, 0xe8, 0x22, 0xbd, 0x6d,
        0xa9, 0x28, 0x35, 0x4e, 0x1c, 0x25, 0xd2, 0x4a, 0xad, 0xab, 0x85, 0x5b,
        0x44, 0xa9, 0xdc, 0x4c, 0x74, 0xce, 0xea, 0xa9, 0xfd, 0xdd, 0x35, 0x29,
        0xc3, 0x83, 0xc1, 0x0d, 0x3a, 0x05, 0x44, 0x7f, 0x0f, 0x44, 0x69, 0x81,
        0x67, 0x82, 0x3e, 0x64, 0xf9, 0x16, 0x45, 0xce, 0x8b, 0x0c, 0x61, 0x3e,
        0x35, 0x8b, 0x06, 0xe3, 0x80, 0x17, 0x27, 0x1c, 0x82, 0xa6, 0xc2, 0x9f,
        0x26, 0x42, 0x51, 0xb3, 0x7e, 0xbf, 0xa2, 0xf6, 0xbf, 0x6e, 0xb3, 0xb3,
        0xb7, 0xce, 0x48, 0x0c, 0x43, 0x2d, 0x2e, 0x16, 0xcf, 0x0b, 0xbe, 0x05,
        0xda, 0xe9, 0x39, 0xfd, 0x73, 0x74, 0x38, 0x8d, 0x48, 0x07, 0x06, 0xfb,
        0x53, 0x02, 0x01, 0x02
    };

    if (_context) {
        return true;
    }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    const SSL_METHOD *const method = TLS_method();
#else
    const SSL_METHOD *const method =
            _enhanced_security ? TLSv1_2_method() : SSLv23_method();
#endif

    const char *const ciphers = _enhanced_security
            ? "ECDH+AESGCM:DH+AESGCM:"
              "ECDH+AES:DH+AES:"
              "!AES256:!SHA:!MD5:!DSS:!aNULL:!eNULL"
            : "ECDH+AESGCM:DH+AESGCM:"
              "ECDH+AES:DH+AES:"
              "ECDH+3DES:DH+3DES:"
              "kRSA+AESGCM:kRSA+AES:kRSA+3DES:"
              "!AES256:!MD5:!DSS:!aNULL:!eNULL";

    const unsigned char *dhparam = kDefaultDHParam;
    DH *dh = d2i_DHparams(NULL, &dhparam, sizeof(kDefaultDHParam));
    if (!dh) {
        return false;
    }

    EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!ecdh) {
        DH_free(dh);
        return false;
    }

    _context = SSL_CTX_new(method);
    if (!_context) {
        EC_KEY_free(ecdh);
        DH_free(dh);
        return false;
    }

    if (SSL_CTX_set_tmp_ecdh(_context, ecdh) != 1 ||
        SSL_CTX_set_tmp_dh(_context, dh) != 1     ){

        SSL_CTX_free(_context);
        _context = NULL;
        EC_KEY_free(ecdh);
        DH_free(dh);
        return false;
    }

    EC_KEY_free(ecdh);
    DH_free(dh);

    if (SSL_CTX_set_cipher_list(_context, ciphers) != 1) {
        SSL_CTX_free(_context);
        _context = NULL;
        return false;
    }

    // Disable session resumptions by default to provide maximum security.
    SSL_CTX_set_session_cache_mode(_context, SSL_SESS_CACHE_OFF);
    SSL_CTX_set_options(_context, SSL_OP_NO_TICKET);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    if (SSL_CTX_set_min_proto_version(_context,
            _enhanced_security ? TLS1_2_VERSION : TLS1_VERSION) != 1) {

        SSL_CTX_free(_context);
        _context = NULL;
        return false;
    }
#else
    SSL_CTX_set_options(_context, SSL_OP_NO_SSLv2);
    SSL_CTX_set_options(_context, SSL_OP_NO_SSLv3);
#endif

    // It's known vulnerable.
    SSL_CTX_set_options(_context, SSL_OP_NO_COMPRESSION);

    // Always use ephemeral keys.
    SSL_CTX_set_options(_context, SSL_OP_SINGLE_DH_USE);
    SSL_CTX_set_options(_context, SSL_OP_SINGLE_ECDH_USE);

    // Listen to me.
    SSL_CTX_set_options(_context, SSL_OP_CIPHER_SERVER_PREFERENCE);

    // Extra settings.
    SSL_CTX_set_mode(_context, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
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

bool SslContext::LoadDHParam(const std::string &filename)
{
    if (filename.empty()) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        return false;
    }

    DH *dh = PEM_read_DHparams(file, NULL, NULL, NULL);
    fclose(file);

    if (!dh) {
        return false;
    }

    if (SSL_CTX_set_tmp_dh(_context, dh) != 1) {
        DH_free(dh);
        return false;
    }

    DH_free(dh);
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

int SslContext::PasswordCallback(char *buf, int size, int /*rwflag*/, void *userdata)
{
    const std::string *s = reinterpret_cast<const std::string *>(userdata);
    if (!s) {
        return -1;
    }

    int ret = snprintf(buf, static_cast<size_t>(size), "%s", s->c_str());
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

bool SslContext::SetVerifyPeer(bool verify, bool required)
{
    if (!Initialize()) {
        return false;
    }

    int mode = verify ? SSL_VERIFY_CLIENT_ONCE
                      | SSL_VERIFY_PEER
                      : SSL_VERIFY_NONE;

    if (required) {
        mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }

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

bool SslContext::SetSessionIdContext(const std::string &context)
{
    if (context.empty() || context.length() > SSL_MAX_SSL_SESSION_ID_LENGTH) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    SSL_CTX_set_session_cache_mode(_context, SSL_SESS_CACHE_SERVER);
    if (SSL_CTX_set_session_id_context(_context,
            reinterpret_cast<const unsigned char *>(context.data()),
            static_cast<unsigned int>(context.length())) != 1) {

        return false;
    }

    return true;
}

bool SslContext::SetSessionTimeout(int seconds)
{
    if (!seconds) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    SSL_CTX_set_timeout(_context, seconds);
    return true;
}

bool SslContext::SetAllowTlsTicket(bool allow)
{
    if (!Initialize()) {
        return false;
    }

    if (allow) {
        SSL_CTX_clear_options(_context, SSL_OP_NO_TICKET);
    } else {
        SSL_CTX_set_options(_context, SSL_OP_NO_TICKET);
    }

    return true;
}

ssize_t SslContext::LoadFile(
        const std::string &filename,
        void *buffer, size_t length)
{
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END)) {
        fclose(fp);
        return -1;
    }

    long size = ftell(fp);
    if (size < 0 || static_cast<size_t>(size) > length) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET)) {
        fclose(fp);
        return -1;
    }

    size_t ret = fread(buffer, 1, static_cast<size_t>(size), fp);
    if (ret != static_cast<size_t>(size)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return size;
}

bool SslContext::LoadTlsTicketKey(const std::string &filename)
{
    if (filename.empty()) {
        return false;
    } else if (!Initialize()) {
        return false;
    }

    char buffer[128];
    ssize_t ret = LoadFile(filename, buffer, sizeof(buffer));
    if (ret != 48 && ret != 80) {
        return false;
    }

    // Initializing static data, a mutex shall be implemented but it's usually
    // not necessary and will slow down every session establishing.
    if (_tls_ticket_key_index < 0) {
        _tls_ticket_key_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
        if (_tls_ticket_key_index < 0) {
            return false;
        }
    }
    // End of initializing static data

    std::vector<key_t> *keys =
            reinterpret_cast<std::vector<key_t> *>(
                    SSL_CTX_get_ex_data(_context, _tls_ticket_key_index));

    if (!keys) {
        keys = new std::vector<key_t>;
        if (SSL_CTX_set_ex_data(_context, _tls_ticket_key_index, keys) != 1) {
            delete keys;
            return false;
        }

        // keys are kept in context and destroyed in dtor.
        if (SSL_CTX_set_tlsext_ticket_key_cb(_context, TlsTicketKeyCallback) != 1) {
            return false;
        }
    }

    key_t key;
    if (ret == 48) {
        key.bits = 128;
        memcpy(key.name, buffer +  0, 16);
        memcpy(key.hmac, buffer + 16, 16);
        memcpy(key.ekey, buffer + 32, 16);
    } else {
        key.bits = 256;
        memcpy(key.name, buffer +  0, 16);
        memcpy(key.hmac, buffer + 16, 32);
        memcpy(key.ekey, buffer + 48, 32);
    }

    keys->push_back(key);
    return true;
}

int SslContext::TlsTicketKeyCallback(
        SSL *s,
        unsigned char *key_name,
        unsigned char *iv,
        EVP_CIPHER_CTX *ctx,
        HMAC_CTX *hctx,
        int enc)
{
#ifdef OPENSSL_NO_SHA256
    const EVP_MD *const digest = EVP_sha1();
#else
    const EVP_MD *const digest = EVP_sha256();
#endif

    SSL_CTX *const context = SSL_get_SSL_CTX(s);
    const std::vector<key_t> *const keys =
            reinterpret_cast<std::vector<key_t> *>(
                    SSL_CTX_get_ex_data(context, _tls_ticket_key_index));

    if (!keys || keys->empty()) {
        CLOG.Error("SSL: BUG session ticket keys are not set");
        return -1;
    }

    if (enc) {
        const key_t *const key = &keys->at(0);
        const EVP_CIPHER *cipher;
        int size;

        if (key->bits == 128) {
            cipher = EVP_aes_128_cbc();
            size = 16;
        } else {
            cipher = EVP_aes_256_cbc();
            size = 32;
        }

        if (RAND_bytes(iv, EVP_CIPHER_iv_length(cipher))         != 1 ||
            EVP_EncryptInit_ex(ctx, cipher, NULL, key->ekey, iv) != 1 ||
            HMAC_Init_ex(hctx, key->hmac, size, digest, NULL)    != 1 ){

            return -1;
        }

        memcpy(key_name, key->name, 16);
        CLOG.Verbose("SSL: new session ticket generated");
        return 1;

    } else {
        const key_t *key;
        bool found = false;
        bool renew = false;
        for (std::vector<key_t>::const_iterator p = keys->begin(); p != keys->end(); ++p) {
            if (memcmp(key_name, p->name, 16) == 0) {
                found = true;
                key = &*p;
                break;
            }

            renew = true;
        }

        if (!found) {
            CLOG.Verbose("SSL: session ticket key name is not found");
            return 0;
        }

        const EVP_CIPHER *cipher;
        int size;

        if (key->bits == 128) {
            cipher = EVP_aes_128_cbc();
            size = 16;
        } else {
            cipher = EVP_aes_256_cbc();
            size = 32;
        }

        if (HMAC_Init_ex(hctx, key->hmac, size, digest, NULL)    != 1 ||
            EVP_DecryptInit_ex(ctx, cipher, NULL, key->ekey, iv) != 1 ){

            return -1;
        }

        CLOG.Verbose("SSL: session ticket %s", renew ? "needs renewal" : "accepted");
        return renew ? 2 : 1;
    }
}

} // namespace flinter

#endif // HAVE_OPENSSL_OPENSSLV_H
