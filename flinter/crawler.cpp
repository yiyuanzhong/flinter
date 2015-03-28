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

#include "flinter/crawler.h"

#include <assert.h>

#include <curl/curl.h>

#include "flinter/logger.h"

namespace flinter {

class Crawler::Context {
public:
    Context() : curl(NULL), slist(NULL) {}
    CURL *curl;
    struct curl_slist *slist;

}; // class Crawler::Context

Crawler::Crawler(const std::string &url, const std::string &hostname)
        : _hostname(hostname), _url(url), _status(0), _context(new Context)
{
    // Intended left blank.
}

Crawler::~Crawler()
{
    if (_context->curl) {
        curl_easy_cleanup(_context->curl);
    }

    if (_context->slist) {
        curl_slist_free_all(_context->slist);
    }

    delete _context;
}

bool Crawler::Get(std::string *result)
{
    if (!result || !Initialize() || !SetMethod(true)) {
        return false;
    }

    return Request(result);
}

bool Crawler::Request(std::string *result)
{
    CURL *curl = _context->curl;
    CURLcode ret = curl_easy_perform(curl);
    if (ret != CURLE_OK) {
        CLOG.Warn("Crawler: failed to perform [%s]: %d: %s",
                  _url.c_str(), ret, curl_easy_strerror(ret));

        return false;
    }

    ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &_status);
    if (ret != CURLE_OK) {
        CLOG.Warn("Crawler: failed to get response code [%s]: %d: %s",
                  _url.c_str(), ret, curl_easy_strerror(ret));

        return false;
    }

    char *raw;
    ret = curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &raw);
    if (ret != CURLE_OK) {
        CLOG.Warn("Crawler: failed to get effective url [%s]: %d: %s",
                  _url.c_str(), ret, curl_easy_strerror(ret));

        return false;
    }

    _effective_url = raw;

    ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &raw);
    if (ret != CURLE_OK) {
        CLOG.Warn("Crawler: failed to get content type [%s]: %d: %s",
                  _url.c_str(), ret, curl_easy_strerror(ret));

        return false;
    }

    _content_type = raw;

    result->assign(_result.begin(), _result.end());
    _result.clear();
    return true;
}

bool Crawler::Post(std::string *result)
{
    if (!result || !Initialize() || !SetMethod(false)) {
        return false;
    }

    struct curl_httppost *last = NULL;
    struct curl_httppost *first = NULL;
    for (std::map<std::string, std::string>::const_iterator p = _posts.begin();
         p != _posts.end(); ++p) {

        CURLFORMcode ret = curl_formadd(&first, &last,
                                        CURLFORM_PTRNAME, p->first.data(),
                                        CURLFORM_NAMELENGTH, p->first.length(),
                                        CURLFORM_PTRCONTENTS, p->second.data(),
                                        CURLFORM_CONTENTSLENGTH, p->second.length(),
                                        CURLFORM_END);

        if (ret != CURL_FORMADD_OK) {
            curl_formfree(first);
            return false;
        }
    }

    if (curl_easy_setopt(_context->curl, CURLOPT_HTTPPOST, first)) {
        curl_formfree(first);
        return false;
    }

    bool ret = Request(result);
    curl_formfree(first);
    return ret;
}

bool Crawler::PostRaw(const std::string &content_type,
                      const std::string &raw,
                      std::string *result)
{
    if (content_type.empty() || !result ||
        !Initialize(content_type)       ||
        !SetMethod(false)               ){

        return false;
    }

    curl_off_t size = raw.length();
    if (curl_easy_setopt(_context->curl, CURLOPT_POSTFIELDS, raw.data())    ||
        curl_easy_setopt(_context->curl, CURLOPT_POSTFIELDSIZE_LARGE, size) ){

        return false;
    }

    return Request(result);
}

void Crawler::Clear()
{
    _posts.clear();
}

void Crawler::Set(const std::string &key, const char *value)
{
    _posts[key] = value;
}

void Crawler::Set(const std::string &key, const std::string &value)
{
    _posts[key] = value;
}

size_t Crawler::WriteFunction(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    Crawler *crawler = reinterpret_cast<Crawler *>(userdata);
    std::deque<char> &result = crawler->_result;
    size_t length = size * nmemb;

    result.insert(result.end(), ptr, ptr + length);
    return length;
}

bool Crawler::Initialize(const std::string &content_type)
{
    if (_context->curl) {
        return true;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L)             ||
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L)                  ||
        curl_easy_setopt(curl, CURLOPT_URL, _url.c_str())              ||
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction)   ||
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this)                ||
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L)             ||
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L)             ||
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L)             ||
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L)                  ){

        curl_easy_cleanup(curl);
        return false;
    }

    struct curl_slist *slist = NULL;
    if (!_hostname.empty()) {
        std::ostringstream s;
        s << "Host: " << _hostname;
        slist = curl_slist_append(slist, s.str().c_str());
    }

    if (!content_type.empty()) {
        std::ostringstream s;
        s << "Content-Type: " << content_type;
        slist = curl_slist_append(slist, s.str().c_str());
    }

    slist = curl_slist_append(slist, "Expect:");
    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist) != CURLE_OK) {
        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
        return false;
    }

    _context->slist = slist;
    _context->curl = curl;
    return true;
}

bool Crawler::SetMethod(bool get_or_post)
{
    assert(_context->curl);

    CURLoption method = get_or_post ? CURLOPT_HTTPGET : CURLOPT_POST;
    CURLcode ret = curl_easy_setopt(_context->curl, method, 1L);
    if (ret != CURLE_OK) {
        return false;
    }

    return true;
}

bool Crawler::SetCertificateAuthorityFile(const std::string &filename)
{
    if (!_context->curl) {
        return false;
    }

    _cainfo = filename;
    CURL *curl = _context->curl;
    if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L)      ||
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L)      ||
        curl_easy_setopt(curl, CURLOPT_CAINFO, _cainfo.c_str()) ){

        return false;
    }

    return true;
}

} // namespace flinter
