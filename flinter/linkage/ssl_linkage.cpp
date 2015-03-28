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

#include "flinter/linkage/ssl_linkage.h"

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <assert.h>
#include <errno.h>

#include "flinter/linkage/linkage_peer.h"
#include "flinter/linkage/linkage_worker.h"
#include "flinter/linkage/ssl_context.h"
#include "flinter/logger.h"

namespace flinter {

SslLinkage::SslLinkage(LinkageHandler *handler,
                       const LinkagePeer &peer,
                       const LinkagePeer &me,
                       SslContext *context) : Linkage(handler, peer, me)
                                            , _ssl(SSL_new(context->context()))
                                            , _in_progress(kActionAccept)
                                            , _shutdown(false)
                                            , _peer_serial_number(0)
{
    if (!SSL_set_fd(_ssl, peer.fd())) {
        SSL_free(_ssl);
        _ssl = NULL;
    }
}

SslLinkage::SslLinkage(LinkageHandler *handler,
                       const std::string &name,
                       SslContext *context) : Linkage(handler, name)
                                            , _ssl(SSL_new(context->context()))
                                            , _in_progress(kActionConnect)
                                            , _shutdown(false)
                                            , _peer_serial_number(0)
{
    // Intended left blank.
}

SslLinkage::~SslLinkage()
{
    if (_ssl) {
        SSL_free(_ssl);
    }
}

const char *SslLinkage::GetActionString(const Action in_progress)
{
    switch (in_progress) {
    case kActionRead:
        return "reading";

    case kActionWrite:
        return "writing";

    case kActionAccept:
        return "accepting";

    case kActionConnect:
        return "connecting";

    case kActionShutdown:
        return "closing";

    default:
        assert(false);
        return NULL;
    };
}

int SslLinkage::HandleError(LinkageWorker *worker,
                            const Action in_progress, int ret)
{
    _in_progress = in_progress;
    const char *actstr = GetActionString(in_progress);
    if (!actstr) {
        return -1;
    }

    int r = SSL_get_error(_ssl, ret);
    if (r == SSL_ERROR_WANT_READ) {
        CLOG.Verbose("Linkage: %s but want to read for fd = %d", actstr, peer()->fd());
        worker->SetWanna(this, true, false);
        return 1;

    } else if (r == SSL_ERROR_WANT_WRITE) {
        CLOG.Verbose("Linkage: %s but want to write for fd = %d", actstr, peer()->fd());
        worker->SetWanna(this, false, true);
        return 1;

    } else if (r == SSL_ERROR_ZERO_RETURN) {
        CLOG.Verbose("Linkage: %s but connection closed for fd = %d", actstr, peer()->fd());
        return 0;

    } else if (r == SSL_ERROR_SYSCALL) {
        if (errno == 0) {
            CLOG.Verbose("Linkage: %s but peer hung for fd = %d", actstr, peer()->fd());
            return 0;
        }

        CLOG.Verbose("Linkage: %s but system error for fd = %d: %d: %s",
                     actstr, peer()->fd(), errno, strerror(errno));

        return -1;

    } else if (r == SSL_ERROR_WANT_X509_LOOKUP) {
        CLOG.Verbose("Linkage: %s but failed to verify peer certificate for fd = %d",
                     actstr, peer()->fd());

        errno = EACCES;
        return -1;

    } else if (r == SSL_ERROR_SSL) {
        unsigned long err = ERR_get_error();
        CLOG.Verbose("Linkage: %s but error occurred for fd = %d: [%d:%d:%d] %s:%s:%s ",
                     actstr, peer()->fd(),
                     ERR_GET_LIB(err), ERR_GET_FUNC(err), ERR_GET_REASON(err),
                     ERR_lib_error_string(err), ERR_func_error_string(err),
                     ERR_reason_error_string(err));

        errno = EPERM;
        return -1;

    } else {
        CLOG.Verbose("Linkage: %s but unknown ret for fd = %d: %d",
                     actstr, peer()->fd(), ret);

        errno = EBADMSG;
        return -1;
    }
}

int SslLinkage::OnEvent(LinkageWorker *worker, bool reading_or_writing)
{
    Action in_progress = _in_progress;
    if (in_progress == kActionNone) {
        if (_shutdown) {
            in_progress = kActionShutdown;
        } else if (reading_or_writing) {
            in_progress = kActionRead;
        } else {
            in_progress = kActionWrite;
        }
    }

    errno = 0;
    if (in_progress == kActionRead) {
        unsigned char buffer[8192];
        int ret = SSL_read(_ssl, buffer, sizeof(buffer));
        if (ret > 0) {
            UpdateLastReceived();
            _in_progress = kActionNone;
            worker->SetWannaRead(this, true);
            return OnReceived(buffer, static_cast<size_t>(ret));
        }

        return HandleError(worker, in_progress, ret);

    } else if (in_progress == kActionWrite) {
        unsigned char buffer[16384];
        size_t length = PickSendingBuffer(buffer, sizeof(buffer));
        if (!length) {
            ClearSendJam();
            _in_progress = kActionNone;
            if (_graceful) {
                return DoShutdown(worker);
            }
            return 1;
        }

        int ret = SSL_write(_ssl, buffer, length);
        if (ret > 0) {
            if (static_cast<size_t>(ret) < length) {
                UpdateSendJam();
                ConsumeSendingBuffer(static_cast<size_t>(ret));
                CLOG.Verbose("Linkage: dequeued [%lu] bytes and sent [%d] "
                             "bytes for fd = %d", length, ret, peer()->fd());

                assert(GetSendingBufferSize());
                worker->SetWannaWrite(this, true);

            } else {
                ClearSendJam();
                ConsumeSendingBuffer(length);
                CLOG.Verbose("Linkage: dequeued [%lu] bytes and sent [%d] "
                             "bytes for fd = %d", length, ret, peer()->fd());

                if (!GetSendingBufferSize()) {
                    if (_graceful) {
                        return DoShutdown(worker);
                    }

                    worker->SetWannaWrite(this, false);
                }
            }

            _in_progress = kActionNone;
            return 1;
        }

        return HandleError(worker, in_progress, ret);

    } else if (in_progress == kActionAccept) {
        CLOG.Verbose("Linkage: SSL handshaking for fd = %d", peer()->fd());
        int ret = SSL_accept(_ssl);
        if (ret == 1) { // Handshake completed.
            _in_progress = kActionNone;
            return OnConnected() ? 1 : -1;
        }

        return HandleError(worker, in_progress, ret);

    } else if (in_progress == kActionConnect) {
        return DoConnect(worker);

    } else if (in_progress == kActionShutdown) {
        return DoShutdown(worker);
    }

    assert(false);
    return -1;
}

int SslLinkage::DoConnect(LinkageWorker *worker)
{
    CLOG.Verbose("Linkage: SSL handshaking for fd = %d", peer()->fd());
    int ret = SSL_connect(_ssl);
    if (ret == 1) { // Handshake completed.
        _in_progress = kActionNone;
        return OnConnected() ? 1 : -1;

    } else if (ret == 0) {
        CLOG.Verbose("Linkage: SSL handshake failed and closed for fd = %d",
                     peer()->fd());

        return 0;
    }

    return HandleError(worker, kActionConnect, ret);
}

int SslLinkage::DoShutdown(LinkageWorker *worker)
{
    CLOG.Verbose("Linkage: SSL shutdown for fd = %d", peer()->fd());
    int ret = SSL_shutdown(_ssl);
    if (ret == 0) {
        CLOG.Verbose("Linkage: SSL unidirectional shutdown for fd = %d", peer()->fd());
        ret = SSL_shutdown(_ssl);
    }

    if (ret == 1) {
        CLOG.Verbose("Linkage: SSL bidirectional shutdown for fd = %d", peer()->fd());
        _in_progress = kActionNone;
        Linkage::Shutdown();
        ClearSendJam();
        return 0;
    }

    return HandleError(worker, kActionShutdown, ret);
}

bool SslLinkage::OnConnected()
{
    if (_in_progress == kActionConnect) {
        int ret = DoConnect(worker());
        if (ret <= 0) {
            return false;
        }

        return true;

    } else if (_in_progress == kActionAccept) {
        return true;
    }

    CLOG.Verbose("Linkage: SSL handshaked with cipher [%s] for fd = %d",
                 SSL_get_cipher(_ssl), peer()->fd());

    if (!GetInformation()) {
        return false;
    }

    if (!Linkage::OnConnected()) {
        CLOG.Warn("Linkage: client failed to initialize for fd = %d",
                  peer()->fd());

        return false;
    }

    worker()->SetWanna(this, true, !!GetSendingBufferSize());
    ClearSendJam();
    return true;
}

int SslLinkage::OnReadable(LinkageWorker *worker)
{
    return OnEvent(worker, true);
}

int SslLinkage::OnWritable(LinkageWorker *worker)
{
    if (_in_progress == kActionConnect) {
        return Linkage::OnWritable(worker);
    }

    return OnEvent(worker, false);
}

bool SslLinkage::Send(const void *buffer, size_t length)
{
    if (_in_progress != kActionNone || GetSendingBufferSize()) {
        AppendSendingBuffer(buffer, length);
        return true;
    }

    int ret = SSL_write(_ssl, buffer, length);
    if (ret > 0) {
        if (static_cast<size_t>(ret) < length) {
            UpdateSendJam();
            worker()->SetWannaWrite(this, true);
            AppendSendingBuffer(reinterpret_cast<const char *>(buffer) + ret, length - ret);
            CLOG.Verbose("Linkage: sent [%d] bytes, queued [%lu] bytes for fd = %d",
                         ret, length - ret, peer()->fd());

        } else {
            ClearSendJam();
            CLOG.Verbose("Linkage: sent [%d] bytes for fd = %d", ret, peer()->fd());
            worker()->SetWannaWrite(this, false);
        }

        return true;
    }

    AppendSendingBuffer(buffer, length);
    ret = HandleError(worker(), kActionWrite, ret);
    if (ret < 0) {
        return false;
    }

    return true;
}

int SslLinkage::Shutdown()
{
    _shutdown = true;
    if (_in_progress != kActionNone) {
        return 1;
    }

    return DoShutdown(worker());
}

SslLinkage *SslLinkage::ConnectTcp4(LinkageHandler *handler,
                                    const std::string &host,
                                    uint16_t port,
                                    SslContext *context)
{
    if (!handler || host.empty() || !port) {
        return NULL;
    }

    SslLinkage *linkage = new SslLinkage(handler, host, context);
    if (!linkage->DoConnectTcp4(host, port)) {
        delete linkage;
        return NULL;
    }

    if (!SSL_set_fd(linkage->_ssl, linkage->peer()->fd())) {
        delete linkage;
        return NULL;
    }

    return linkage;
}

SslLinkage *SslLinkage::ConnectUnix(LinkageHandler *handler,
                                    const std::string &sockname,
                                    bool file_based,
                                    SslContext *context)
{
    if (!handler || sockname.empty()) {
        return NULL;
    }

    SslLinkage *linkage = new SslLinkage(handler, sockname, context);
    if (!linkage->DoConnectUnix(sockname, file_based)) {
        delete linkage;
        return NULL;
    }

    if (!SSL_set_fd(linkage->_ssl, linkage->peer()->fd())) {
        delete linkage;
        return NULL;
    }

    return linkage;
}

bool SslLinkage::GetInformation()
{
    X509 *x509 = SSL_get_peer_certificate(_ssl);
    if (!x509) {
        CLOG.Verbose("Linkage: no peer certificate for fd = %d", peer()->fd());
        return true;
    }

    char buffer[8192];
    CLOG.Verbose("Linkage: peer certificate detected for fd = %d", peer()->fd());

    if (!X509_NAME_oneline(X509_get_subject_name(x509), buffer, sizeof(buffer))) {
        return false;
    }

    _peer_subject_name = buffer;

    if (!X509_NAME_oneline(X509_get_issuer_name(x509), buffer, sizeof(buffer))) {
        return false;
    }

    _peer_issuer_name = buffer;
    _peer_serial_number = static_cast<uint64_t>(ASN1_INTEGER_get(
                X509_get_serialNumber(x509)));

    return true;
}

} // namespace flinter
