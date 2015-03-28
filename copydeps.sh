#! /bin/sh

CWD=`dirname "${0}"` || exit 1
CWD=`cd "${CWD}" && pwd` || exit 1
ROOT=`cd "${CWD}/.." && pwd` || exit 1
THIRDPARTY="${CWD}/../thirdparty/staging/lib"

if [ $# -ne 2 ]; then
    echo "Usage: ${0} <so filename> <target directory>"
    exit 1
fi

export LD_LIBRARY_PATH="${THIRDPARTY}":"${CWD}/output/lib":"${2}":"${LD_LIBRARY_PATH}"

SYSTEM=`uname -s`
if [ x"${SYSTEM}" != x"Linux" ]; then
    echo "Not supported." >&2
    exit 1
fi

list=`ldd "${1}"` || exit 1
files=`echo "${list}" | awk '{
    if (match($3, "^'"${ROOT}"'")) {
        print $3;
    } else if ($3 == "not") {
        exit(1);
    }
}'` || exit 1

for i in ${files}; do
    r=`readlink -e "${i}"` || exit 1
    if [ ! "${r}" -ef "${2}"/`basename "${r}"` ]; then
        cp -af "${r}" "${2}" || exit 1
    fi
    strip -S "${2}"/`basename "${r}"` || exit 1
    if [ -L "${i}" ]; then
        ln -sf `basename "${r}"` "${2}"/`basename "${i}"` || exit 1
    fi
done

exit 0
