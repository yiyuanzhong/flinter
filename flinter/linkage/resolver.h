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

#ifndef __FLINTER_LINKAGE_RESOLVER_H__
#define __FLINTER_LINKAGE_RESOLVER_H__

#include <sys/types.h>
#include <stdint.h>
#include <time.h>

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <flinter/thread/mutex.h>
#include <flinter/singleton.h>

struct addrinfo;
struct hostent;
struct sockaddr;

namespace flinter {

/// Resolver is a hostname to IPv4 cache.
/// @warning srand(3) before using any random based resolving.
class Resolver : public Singleton<Resolver> {
    DECLARE_SINGLETON(Resolver);

public:
    enum Option {
        kFirst,
        kRandom,
        kSequential,
    }; // enum Option

    /*
     * Resolve hostname to IPs and get desired result instructed by option. Cache applies.
     *
     * @param hostname hostname to resolve.
     * @param result   resolved ip in string form.
     * @param option   resolving option.
     * @param expire   seconds before resolve again, must be positive.
     *                 only effective the first time it's resolved, or after clear().
     */
    bool Resolve(const std::string &hostname,
                 std::string *result,
                 const Option &option = kRandom,
                 time_t expire = kDefaultRefreshTime);

    /*
     * Resolve hostname to IPs and get all results. Cache applies.
     *
     * @param hostname hostname to resolve.
     * @param result   resolved ip in string form.
     * @param expire   seconds before resolve again, must be positive.
     *                 only effective the first time it's resolved, or after clear().
     */
    bool Resolve(const std::string &hostname,
                 std::list<std::string> *result,
                 time_t expire = kDefaultRefreshTime);

    /// Remove an IP from cache, causing sequential call to resolve() not returning that IP.
    /// Effect will be undone when expire time is reached.
    void Invalidate(const std::string &hostname,
                    const std::string &ip,
                    time_t expire = kDefaultBlacklistTime);

    /// Invalidate all cache immediately.
    void Clear();

    /// Invalidate cache for the host immediately.
    void Clear(const std::string &hostname);

    /// Use getaddrinfo(3), which conform to RFC3484 section 6.
    void SetResolveMode(bool use_getaddrinfo)
    {
        _use_getaddrinfo = use_getaddrinfo;
    }

    static const time_t kDefaultRefreshTime;   ///< How long to resolve again for a host.
    static const time_t kDefaultBlacklistTime; ///< How long to restore a blacklisted IP.

private:
    typedef std::string Key;

    class Value {
    public:
        Value(struct addrinfo *res);
        Value(struct hostent *res);

        std::set<std::string> _dns;         ///< DNS result.
        std::vector<std::string> _addr;     ///< Valid IPs.

        /// Invalid IPs and when restore it again.
        std::list<std::pair<std::string, int64_t> > _bad;

        int64_t _resolved;  ///< Last time when I resolve in real.
        int64_t _expire;    ///< Seconds that need resolve again.
        int64_t _active;    ///< Last time when someone resolved.
        size_t _current;    ///< Current index in sequential mode.

    }; // class Value

    /// Clean IP address.
    static bool CleanupIp(const std::string &ip, std::string *result);

    /// Clean hostname.
    static bool CleanupHostname(const std::string &hostname, std::string *result);

    Resolver();
    ~Resolver() {}

    /// @return either cache is valid or resolved successfully.
    Value *DoResolve(const std::string &hostname, int64_t now, int64_t expire);
    Value *DoResolve_gethostbyname(const std::string &hostname);
    Value *DoResolve_getaddrinfo(const std::string &hostname);

    void Aging(int64_t now);

    static const int64_t kCacheExpire;          ///< No one resolves and purge the host.
    static const int64_t kAgingInterval;        ///< How many seconds that we purge.

    typedef std::map<Key, Value> addresses_t;
    addresses_t _addresses;
    bool _use_getaddrinfo;
    int64_t _last_aging;
    Mutex _mutex;

}; // class Resolver

} // namespace flinter

#endif // __FLINTER_LINKAGE_RESOLVER_H__
