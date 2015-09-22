# Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


AC_PREREQ([2.59])
AC_INIT([flinter], [1.0.0], [yiyuanzhong@gmail.com])
AC_CONFIG_FILES([Makefile flinter/Makefile flinter/fastcgi/main/Makefile])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_LANG([C])
AC_CHECK_DECL([__MACH__], [AC_SUBST([CPPFLAGS_APPLE], [-D__unix__=1])])
AX_CHECK_COMPILE_FLAG([-std=gnu11], [AC_SUBST([CFLAGS_11], [-std=gnu11])],
    [AX_CHECK_COMPILE_FLAG([-std=gnu99], [AC_SUBST([CFLAGS_11], [-std=gnu99])])])

AC_LANG([C++])
AX_CHECK_COMPILE_FLAG([-std=gnu++11], [AC_SUBST([CXXFLAGS_11], [-std=gnu++11])],
    [AX_CHECK_COMPILE_FLAG([-std=gnu++0x], [AC_SUBST([CXXFLAGS_11], [-std=gnu++0x])])])

AX_CHECK_COMPILE_FLAG([-Wsign-conversion],
                      [AC_SUBST([CXXFLAGS_SC], [-Wsign-conversion])])

AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--enable-debug],
                              [enable debug build parameters [default=no]])])

AM_CONDITIONAL([ENABLE_DEBUG], [test x"${enable_debug}" = x"yes"])

# OpenSSL
AC_ARG_WITH([openssl],
            [AC_HELP_STRING([--without-openssl],
                            [enable cryptography libraries [default=yes]])])

AM_CONDITIONAL([WITH_OPENSSL], [test x"${with_openssl}" != x"no"])
AS_IF([test x"${with_openssl}" != x"no"],
      [PKG_CHECK_MODULES([openssl], [openssl])
       AC_CHECK_HEADERS([openssl/opensslv.h])])

# libxml2
AC_ARG_WITH([libxml2],
            [AC_HELP_STRING([--without-libxml2],
                            [enable XML libraries [default=yes]])])

AM_CONDITIONAL([WITH_LIBXML2], [test x"${with_libxml2}" != x"no"])
AS_IF([test x"${with_libxml2}" != x"no"],
      [PKG_CHECK_MODULES([libxml2], [libxml-2.0])
       old_CPPFLAGS="${CPPFLAGS}"
       CPPFLAGS="${CPPFLAGS} ${libxml2_CFLAGS}"
       AC_CHECK_HEADERS([libxml/xmlversion.h])
       CPPFLAGS="${old_CPPFLAGS}"])

# libuuid
AC_ARG_WITH([libuuid],
            [AC_HELP_STRING([--without-libuuid],
                            [enable UUID libraries [default=yes]])])

AM_CONDITIONAL([WITH_LIBUUID], [test x"${with_libuuid}" != x"no"])
AS_IF([test x"${with_libuuid}" != x"no"],
      [PKG_CHECK_MODULES([libuuid], [uuid])
       AC_CHECK_HEADERS([uuid/uuid.h])])

# libcurl
AC_ARG_WITH([libcurl],
            [AC_HELP_STRING([--without-libcurl],
                            [enable CURL libraries [default=yes]])])

AM_CONDITIONAL([WITH_LIBCURL], [test x"${with_libcurl}" != x"no"])
AS_IF([test x"${with_libcurl}" != x"no"],
      [PKG_CHECK_MODULES([libcurl], [libcurl])
       AC_CHECK_HEADERS([curl/curl.h])])

# libzookeeper
AC_ARG_WITH([zookeeper],
            [AC_HELP_STRING([--without-zookeeper],
                            [enable ZooKeeper libraries [default=yes]])])

AM_CONDITIONAL([WITH_ZOOKEEPER], [test x"${with_zookeeper}" != x"no"])
AS_IF([test x"${with_zookeeper}" != x"no"],
      [AC_CHECK_HEADERS([zookeeper/zookeeper.h], [],
      [AC_MSG_ERROR([ZooKeeper is missing, try --without-zookeeper])])])

# And fastcgi related
AC_ARG_WITH([fastcgi],
            [AC_HELP_STRING([--without-fastcgi],
                            [build fastcgi library [default=yes]])])

AM_CONDITIONAL([WITH_FASTCGI], [test x"${with_fastcgi}" != x"no"])
AS_IF([test x"${with_fastcgi}" != x"no"],
      [AC_CHECK_HEADERS([ClearSilver/ClearSilver.h], [],
      [AC_MSG_ERROR([ClearSilver is missing, try --without-fastcgi])])])

AS_IF([test x"${with_fastcgi}" != x"no"],
      [AC_CHECK_HEADERS([fcgiapp.h], [],
      [AC_MSG_ERROR([libfcgi is missing, try --without-fastcgi])])])

# All other thirdparty libraries.

AC_CHECK_HEADERS([json/value.h], [AC_SUBST([JSONCPP_LIBS], [-ljsoncpp])])
AC_CHECK_HEADERS([ev.h], [AC_SUBST([EV_LIBS], [-lev])])

# End of all other thirdparty libraries

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(time_t)
AC_CHECK_SIZEOF(void *)

AC_CHECK_HEADERS([sys/param.h sys/cpuset.h], [], [],
                 [[#if HAVE_SYS_PARAM_H
                   # include <sys/param.h>
                   #endif]])

AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/epoll.h])
AC_CHECK_HEADERS([sys/event.h])
AC_CHECK_HEADERS([sys/poll.h])
AC_CHECK_HEADERS([libutil.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([pty.h])
AC_CHECK_HEADERS([semaphore.h])
AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([utmp.h])

AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([libiconv], [iconv])

AC_CHECK_FUNCS([ppoll])
AC_CHECK_FUNCS([openat])
AC_CHECK_FUNCS([pselect])
AC_CHECK_FUNCS([mkdirat])
AC_CHECK_FUNCS([fstatat])
AC_CHECK_FUNCS([unlinkat])
AC_CHECK_FUNCS([fdopendir])
AC_CHECK_FUNCS([epoll_pwait])

AC_CHECK_FUNCS([clearenv])
AC_CHECK_FUNCS([getpeereid])
AC_CHECK_FUNCS([sched_yield])
AC_CHECK_FUNCS([setproctitle])
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([gethostbyname2_r])
AC_CHECK_FUNCS([sched_setaffinity])
AC_CHECK_FUNCS([cpuset_setaffinity])

AC_OUTPUT
