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


AUTOMAKE_OPTIONS = foreign nostdinc
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = flinter

AM_CPPFLAGS = -I${abs_top_builddir} -I${abs_top_srcdir} @CPPFLAGS_APPLE@
AM_CFLAGS = -g -pthread -Wall -Wextra -Winit-self -Wunused -Wpointer-arith -Wconversion @CFLAGS_11@
AM_CXXFLAGS = -g -pthread -Wall -Wextra -Winit-self -Wunused -Wpointer-arith -Wconversion @CXXFLAGS_SC@ @CXXFLAGS_11@

if !ENABLE_DEBUG
AM_CPPFLAGS += -DNDEBUG
AM_CXXFLAGS += -O2
AM_CFLAGS += -O2
endif

if WITH_OPENSSL
AM_CPPFLAGS += ${openssl_CFLAGS}
endif

if WITH_LIBCURL
AM_CPPFLAGS += ${libcurl_CFLAGS}
endif

if WITH_LIBXML2
AM_CPPFLAGS += ${libxml2_CFLAGS}
endif

if WITH_LIBUUID
AM_CPPFLAGS += ${libuuid_CFLAGS}
endif

export AM_CFLAGS
export AM_CPPFLAGS
export AM_CXXFLAGS
