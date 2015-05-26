#! /bin/sh

if [ $# -ne 2 ]; then
    echo "${0} <loader path> <libdir>"
    exit 2
fi

LIBDIR="${2}"
BINDIR=`dirname "${1}"` || exit 2
BINDIR=`cd "${BINDIR}" && pwd` || exit 2
BINNAME=`basename "${1}"` || exit 2
BINNAME="${BINDIR}/${BINNAME}"

SYSTEM=`uname -s` || exit 1
THIRDPARTY=`dirname "${0}"` || exit 1
THIRDPARTY=`cd "${THIRDPARTY}/../thirdparty" && pwd` || exit 1
CHRPATH="${THIRDPARTY}/tools/bin/chrpath"

linux()
{
    elf=`readelf -d "${BINNAME}" 2>/dev/null`
    if [ $? -ne 0 ]; then # Nothing I can do.
        return 0
    fi

    rpath=`echo "${elf}" | awk '{ if ($2 == "(RPATH)") { print substr($5, 2, length($5) - 2); exit(0); } }'`
    runpath=`echo "${elf}" | awk '{ if ($2 == "(RUNPATH)") { print substr($5, 2, length($5) - 2); exit(0); } }'`

    if [ -z "${runpath}" ] && [ -z "${rpath}" ]; then
        return 0
    fi

    if [ -n "${runpath}" ]; then
        ${CHRPATH} -r "${LIBDIR}" "${BINNAME}" || return 1
    else
        ${CHRPATH} -c -r "${LIBDIR}" "${BINNAME}" || return 1
    fi

    return 0
}

macosx()
{
    return 0
}

case "${SYSTEM}" in
    Linux)
        linux
        ;;
    Darwin)
        macosx
        ;;
    *)
        echo "Unsupported machine: ${SYSTEM}"
        false
        ;;
esac
exit $?
