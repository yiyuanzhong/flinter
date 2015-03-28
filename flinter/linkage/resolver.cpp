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

#ifdef __unix__

#include "flinter/linkage/resolver.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "flinter/thread/mutex_locker.h"
#include "flinter/logger.h"
#include "flinter/trim.h"
#include "flinter/utility.h"

#include "config.h"

namespace flinter {

const time_t Resolver::DEFAULT_REFRESH_TIME     = 300;              ///< 5min
const time_t Resolver::DEFAULT_BLACKLIST_TIME   = 60;               ///< 1min

const int64_t Resolver::CACHE_EXPIRE            = 3600000000000LL;  ///< 1h
const int64_t Resolver::AGING_INTERVAL          = 300000000000LL;   ///< 5min

/*
 * I personally prefer getaddrinfo(3) over gethostbyname(3), however GLIBC follows
 *         RFC3484 section 6: Destination Address Selection, which sorts IP addresses
 *         according to the distance of netmask. CDN relies on the order DNS returns
 *         thus a sorting will break it severely.
 *
 * gethostbyname(3) will not sort the result.
 *
 */
Resolver::Value *Resolver::DoResolve_getaddrinfo(const std::string &hostname)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags    = AI_ADDRCONFIG;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    int ret = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
    if (ret || !res) {
        CLOG.Trace("Resolver: failed to resolve [%s]: %d: %s",
                   hostname.c_str(), ret, gai_strerror(ret));

        return NULL;
    }

    Value *value = new Value(res);
    freeaddrinfo(res);
    return value;
}

Resolver::Value *Resolver::DoResolve_gethostbyname(const std::string &hostname)
{
#if HAVE_GETHOSTBYNAME2_R
    int ret_errno;
    char buffer[4096];
    struct hostent *res;
    struct hostent result;
    int ret = gethostbyname2_r(hostname.c_str(), AF_INET, &result,
                               buffer, sizeof(buffer), &res, &ret_errno);

    if (ret || !res) {
        // What can I say? hstrerror(3) is obsolete but no replacement is made.
        // LOG.trace("Resolver: failed to resolve [%s]: %d: %s",
        //           hostname.c_str(), ret_errno, hstrerror(ret_errno));

        CLOG.Trace("Resolver: failed to resolve [%s]: %d", hostname.c_str(), ret_errno);
        return NULL;
    }

    Value *value = new Value(res);
    return value;
#else
    (void)hostname;
    return NULL;
#endif
}

Resolver::Value *Resolver::DoResolve(const std::string &hostname, int64_t now, int64_t expire)
{
    addresses_t::iterator p = _addresses.find(hostname);
    if (p != _addresses.end()) {
        Value &v = p->second;

        // Try to recover some of the invalidated IPs first.
        for (std::list<std::pair<std::string, int64_t> >::iterator q = v._bad.begin();
             q != v._bad.end();) {

            if (now < q->second) {
                break;
            }

            v._addr.push_back(q->first);
            q = v._bad.erase(q);
        }

        if (now - p->second._resolved < p->second._expire) {
            // Cache hit.
            return &v;
        }
    }

    // Cache miss.

    Value *value;
#if HAVE_GETHOSTBYNAME2_R
    if (_use_getaddrinfo) {
        value = DoResolve_getaddrinfo(hostname);
    } else {
        value = DoResolve_gethostbyname(hostname);
    }
#else
    value = DoResolve_getaddrinfo(hostname);
#endif

    if (!value) {
        return NULL;
    }

    value->_resolved = now;
    value->_expire = expire;
    value->_active = now;

    CLOG.Verbose("Resolver: resolved [%s] to %lu result(s).",
                 hostname.c_str(), value->_addr.size());

    // It's (very likely) possible that the result is the same.
    if (p != _addresses.end()) {
        if (p->second._dns == value->_dns) {
            // Cool, keep it alive.
            p->second._resolved = now;
            delete value;
            return &p->second;
        }

        // Oops, reset it.
        _addresses.erase(p);
    }

    p = _addresses.insert(addresses_t::value_type(hostname, *value)).first;
    delete value;
    return &p->second;
}

bool Resolver::Resolve(const std::string &hostname,
                       std::list<std::string> *result,
                       time_t expire)
{
    if (!result || expire <= 0) {
        return false;
    }

    const int64_t now = get_monotonic_timestamp();

    std::string host;
    if (!CleanupHostname(hostname, &host)) {
        return false;
    }

    MutexLocker locker(&_mutex);
    Aging(now); // Save the memory, save the world.

    Value *value = DoResolve(host, now, 1000000000LL * expire);
    if (!value) {
        return false;
    }

    result->clear();
    for (std::vector<std::string>::const_iterator p = value->_addr.begin();
         p != value->_addr.end(); ++p) {

        result->push_back(*p);
    }

    return true;
}

bool Resolver::Resolve(const std::string &hostname,
                       std::string *result,
                       const Option &option,
                       time_t expire)
{
    if (!result || expire <= 0) {
        return false;
    } else if (option != FIRST && option != RANDOM && option != SEQUENTIAL) {
        return false;
    }

    const int64_t now = get_monotonic_timestamp();

    std::string host;
    if (!CleanupHostname(hostname, &host)) {
        return false;
    }

    MutexLocker locker(&_mutex);
    Aging(now); // Save the memory, save the world.

    Value *value = DoResolve(host, now, 1000000000LL * expire);
    if (!value) {
        return false;
    }

    value->_active = now;
    if (value->_addr.empty()) {
        return false;
    }

    if (option == FIRST) {
        *result = value->_addr[0];
        return true;

    } else if (option == RANDOM) {
        // This one shouldn't be negative but I just protect it.
        int r = rand();
        r = r < 0 ? -r : r;
        size_t index = static_cast<size_t>(r) % value->_addr.size();
        *result = value->_addr[index];
        return true;

    } else if (option == SEQUENTIAL) {
        *result = value->_addr[value->_current];

        ++value->_current;
        value->_current %= value->_addr.size();
        return true;
    }

    assert(false);
    return false;
}

bool Resolver::CleanupHostname(const std::string &hostname, std::string *result)
{
    assert(result);
    std::string input = trim(hostname);
    if (input.empty()) {
        return false;
    }

    result->resize(input.length());
    std::transform(input.begin(), input.end(), result->begin(), tolower);
    return true;
}

bool Resolver::CleanupIp(const std::string &ip, std::string *result)
{
    assert(result);
    std::string input = trim(ip);

    unsigned long ul[4];
    const char *start = input.c_str();
    char *end;

    for (size_t i = 0; i < 3; ++i) {
        ul[i] = strtoul(start, &end, 10);
        if (end == start || ul[i] >= 256 || *end != '.') {
            return false;
        }

        start = end + 1;
    }

    ul[3] = strtoul(start, &end, 10);
    if (end == start || ul[3] >= 256 || *end != '\0') {
        return false;
    }

    char buffer[INET_ADDRSTRLEN];
    sprintf(buffer, "%lu.%lu.%lu.%lu", ul[0], ul[1], ul[2], ul[3]);
    *result = buffer;
    return true;
}

void Resolver::Invalidate(const std::string &hostname, const std::string &ip, time_t expire)
{
    if (expire <= 0) {
        return;
    }

    std::string host = trim(hostname);
    if (!host.length()) {
        return;
    }

    std::transform(host.begin(), host.end(), host.begin(), tolower);

    std::string value;
    if (!CleanupIp(ip, &value)) {
        return;
    }

    MutexLocker locker(&_mutex);
    addresses_t::iterator p = _addresses.find(host);
    if (p == _addresses.end()) {
        return;
    }

    std::vector<std::string> &v = p->second._addr;
    std::vector<std::string>::iterator q = v.begin();
    size_t &c = p->second._current;
    for (size_t i = 0; i < v.size(); ++i, ++q) {
        if (v[i] == value) {
            const int64_t now = get_monotonic_timestamp();
            const int64_t when = now + 1000000000LL * expire;

            // Put it in queue.
            p->second._bad.push_back(std::pair<std::string, int64_t>(*q, when));

            // Yes, sure, std::vector::erase() is slow, but who cares?
            v.erase(q);

            if (c >= i) {
                if (v.empty()) {
                    break;
                } else if (c == 0) {
                    c = v.size() - 1;
                } else {
                    --c;
                }
            }

            break;
        }
    }
}

void Resolver::Clear()
{
    MutexLocker locker(&_mutex);
    _addresses.clear();
}

// Call locked.
void Resolver::Aging(int64_t now)
{
    // Don't make it too frequent.
    if (now - _last_aging < AGING_INTERVAL) {
        return;
    }
    _last_aging = now;

    size_t count = 0;
    for (addresses_t::iterator p = _addresses.begin(); p != _addresses.end();) {
        addresses_t::iterator q = p++;
        if (now - q->second._active >= CACHE_EXPIRE) {
            _addresses.erase(q);
            ++count;
        }
    }

    if (count) {
        CLOG.Verbose("Resolver: aged %lu hosts.", count);
    }
}

void Resolver::Clear(const std::string &hostname)
{
    std::string host = trim(hostname);
    if (!host.length()) {
        return;
    }

    std::transform(host.begin(), host.end(), host.begin(), tolower);
    MutexLocker locker(&_mutex);
    _addresses.erase(host);
}

Resolver::Resolver() : _use_getaddrinfo(true), _last_aging(get_monotonic_timestamp())
{
    // Intended left blank.
}

Resolver::Value::Value(struct addrinfo *res) : _current(0)
{
    if (!res) {
        return;
    }

    for (struct addrinfo *p = res; p; p = p->ai_next) {
        char buffer[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET,
                      &reinterpret_cast<struct sockaddr_in *>(p->ai_addr)->sin_addr,
                      buffer, sizeof(buffer))) {

            // Yes, sure, std::vector::push_back() is slow, but who cares?
            std::string ip = buffer;
            _dns.insert(ip);

            // Keep the order to allow CDN to work.
            _addr.push_back(ip);
        }
    }
}

Resolver::Value::Value(struct hostent *res) : _current(0)
{
    if (!res || !res->h_addr_list) {
        return;
    }

    for (char **p = res->h_addr_list; *p; ++p) {
        char buffer[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, *p, buffer, sizeof(buffer))) {
            // Yes, sure, std::vector::push_back() is slow, but who cares?
            std::string ip = buffer;
            _dns.insert(ip);

            // Keep the order to allow CDN to work.
            _addr.push_back(ip);
        }
    }
}

} // namespace flinter

#endif // __unix__
