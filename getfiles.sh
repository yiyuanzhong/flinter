#! /bin/sh

CWD=`dirname "${0}"` || exit 1
CWD=`cd "${CWD}" && pwd` || exit 1
THIRDPARTY="${CWD}/../thirdparty/staging/lib"

if [ $# -ne 1 ]; then
    echo "Usage: ${0} <target directory>"
    exit 1
fi

SYSTEM=`uname -s`
if [ x"${SYSTEM}" = x"Darwin" ]; then
    exit 0
fi

cp -af "${CWD}"/output/lib/libflinter_fastcgi_main.so* "${1}/" || exit 1
cp -af "${CWD}"/output/lib/libflinter_core.so* "${1}/" || exit 1
cp -af "${CWD}"/output/lib/libflinter.so* "${1}/" || exit 1
"${CWD}/copydeps.sh" "${1}"/libflinter.so.0 "${1}/" || exit 1
exit 0
