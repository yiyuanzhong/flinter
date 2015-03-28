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


DisableStackProtector = 0
ReleaseAsDebug = 0

### MODIFY ANYTHING BELOW THIS LINE THAT IS SUITABLE ###

TARGET = ../../cgi-bin/big.cgi
MODULEROOT = ../../..
MODULEPATH = flinter

SUBDIRS = .

# "d" will be appended for each STATICS, ABC will become -lABCd eventually
STATICS = flinter/lib/flinter
LIBRARIES = crypto curl fcgi gsoapssl++ jsoncpp neo_cs neo_cgi neo_utl pthread rt ssl z
PKGCONFIGS =

BINPATH =

SEARCHPATH = lamp/staging/include/mysql \
             lamp/staging/include

LIBRARYPATH = lamp/staging/lib/mysql \
              lamp/staging/lib

# Installing related
DESTDIR =
prefix = /usr
EXTRA_INSTALLS =

### MODIFY ANYTHING BELOW THIS LINE CAREFULLY ###

CROSS_COMPILE =

### DON'T MODIFY ANYTHING BELOW THIS LINE ###

GOAL = $(EXECUTABLE)
GOALd = $(EXECUTABLEd)

include ../../include.mk
