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


ReleaseAsDebug = 0
EnableProfiling = 0
StaticLinkLibraries = 1
DisableStackProtector = 0

### MODIFY ANYTHING BELOW THIS LINE THAT IS SUITABLE ###

TARGET = ../test-bin
MODULEROOT = ../..
MODULEPATH = flinter/test

SUBDIRS = .

# "d" will be appended for each STATICS, ABC will become -lABC_debug eventually
STATICS = flinter/output/lib/flinter
LIBRARIES = crypto dl ev gmp iconv icudata icuuc jsoncpp lzma neo_cgi neo_cs \
            neo_utl pthread ssl xml2 z zookeeper_mt gtest gmock gmock_main
PKGCONFIGS =

BINPATH =

SEARCHPATH = thirdparty/staging/include \
             flinter/output/include

LIBRARYPATH = thirdparty/staging/lib

# Installing related
DESTDIR =
prefix = /usr
EXTRA_INSTALLS =

### MODIFY ANYTHING BELOW THIS LINE CAREFULLY ###

CROSS_COMPILE =

### DON'T MODIFY ANYTHING BELOW THIS LINE ###

GOAL = $(TEST)
GOALd = $(TESTd)

include ../../flinter/include.mk
