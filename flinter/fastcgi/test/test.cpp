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

#include "flinter/fastcgi/passport/passport.h"
#include "flinter/fastcgi/cgi.h"
#include "flinter/fastcgi/controller.h"
#include "flinter/fastcgi/common_filters.h"
#include "flinter/fastcgi/dispatcher.h"
#include "flinter/tree.h"

class TestCGI : public flinter::CGI {
protected:
    virtual void Run()
    {
        SetCookie("k", "v!/v", 3600, "HttpOnly");
        SetBodyBuffered(true);
        OutputBody("hello world!\n");
        OutputBodyF("hello wflinterssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssrld!\n");

        OutputBody("SERVER\n");
        for (flinter::Tree::const_iterator p = _SERVER.begin(); p != _SERVER.end(); ++p) {
            OutputBodyF("%s = %s\n", p->first.c_str(), p->second.c_str());
        }

        OutputBody("GET\n");
        for (flinter::Tree::const_iterator p = _GET.begin(); p != _GET.end(); ++p) {
            OutputBodyF("%s = %s\n", p->first.c_str(), p->second.c_str());
        }

        OutputBody("POST\n");
        for (flinter::Tree::const_iterator p = _POST.begin(); p != _POST.end(); ++p) {
            OutputBodyF("%s = %s\n", p->first.c_str(), p->second.c_str());
        }

        OutputBody("REQUEST\n");
        for (flinter::Tree::const_iterator p = _REQUEST.begin(); p != _REQUEST.end(); ++p) {
            OutputBodyF("%s = %s\n", p->first.c_str(), p->second.c_str());
        }

        OutputBody("FILE\n");
        for (std::map<std::string, CGI::File>::const_iterator p = _FILES.begin(); p != _FILES.end(); ++p) {
            OutputBodyF("%s = (%s) (%p) (%ld) [%s] [%s]\n", p->first.c_str(), p->second.name().c_str(), p->second.fp(), p->second.size(), p->second.tmp_name().c_str(), p->second.type().c_str());
        }

        OutputBody("COOKIE\n");
        for (flinter::Tree::const_iterator p = _COOKIE.begin(); p != _COOKIE.end(); ++p) {
            OutputBodyF("%s = %s\n", p->first.c_str(), p->second.c_str());
        }

        if (_GET.Has("kkk")) {
            Redirect("/cgi-bin/env.cgi", false);
        }
    }

}; // class TestCGI

class DefaultCGI : public flinter::CGI {
protected:
    virtual void Run()
    {
        SetStatusCode(404);
        OutputBody("404\n");
    }

};

class streamCGI : public flinter::CGI {
protected:
    virtual void Run()
    {
        SetContentType("text/plain");
        BODY << "oops\n";
        BODY << "hello " << 5;
        std::endl(BODY);
        BODY << "world " << 7 << std::endl;
        End();
        BODY << "not see";
    }
};

class csCGI : public flinter::Controller {
protected:
    virtual void Run()
    {
        flinter::Tree t;
        t["a"]["b"] = time(NULL);
        Render("../html_tpl/tree.cs", t);
    }

};

class sslCGI : public flinter::CGI {
public:
    sslCGI()
    {
        AppendFilter(new flinter::HttpsFilter);
    }

protected:
    virtual void Run()
    {
        BODY << "hello ssl";
    }
private:
};

class Bad : public flinter::CGI {
protected:
    virtual void Run()
    {
        throw flinter::HttpException(400);
    }
};

class Index : public flinter::Controller {
public:
    Index()
    {
        RequireAuthentication();
    }

protected:
    void Run()
    {
        BODY << "<h1>It works!</h1>";
    }
};

CGI_DISPATCH(Index, "/cgi-bin/index.cgi");
CGI_DISPATCH(Bad, "/cgi-bin/bad.cgi");

CGI_DISPATCH(sslCGI, "/cgi-bin/ssl.cgi");
CGI_DISPATCH(streamCGI, "/cgi-bin/stream.cgi");
CGI_DISPATCH(csCGI, "/cgi-bin/cs.cgi");
CGI_DISPATCH(TestCGI, "/cgi-bin/test.cgi");
CGI_DISPATCH(TestCGI, "/cgi-bin/test2.cgi");
//CGI_DISPATCH_DEFAULT(DefaultCGI);
