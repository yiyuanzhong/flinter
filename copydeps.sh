#! /bin/sh

CWD=`dirname "${0}"` || exit 1
CWD=`cd "${CWD}" && pwd` || exit 1
ROOT=`cd "${CWD}/.." && pwd` || exit 1
LIBDIR=`cd "${2}" && pwd` || exit 1
BINDIR=`dirname "${1}"` || exit 1
BINDIR=`cd "${BINDIR}" && pwd` || exit 1
BINFILE=`basename "${1}"` || exit 1
BINNAME="${BINDIR}/${BINFILE}"

if [ $# -ne 2 ]; then
    echo "Usage: ${0} <so filename> <target directory>"
    exit 1
fi

FLINTER="${ROOT}/flinter/output/lib"
THIRDPARTY="${ROOT}/thirdparty/staging/lib"

SYSTEM=`uname -s`

linux()
{
    export LD_LIBRARY_PATH="${THIRDPARTY}:${FLINTER}:${LIBDIR}:${LD_LIBRARY_PATH}"

    list=`ldd "${1}"` || return 1
    files=`echo "${list}" | awk '{
        if (match($3, "^'"${ROOT}"'")) {
            print $3;
        } else if ($3 == "not") {
            exit(1);
        }
    }'` || return 1

    for i in ${files}; do
        r=`readlink -e "${i}"` || return 1
        if [ ! "${r}" -ef "${2}"/`basename "${r}"` ]; then
            cp -af "${r}" "${2}" || return 1
        fi
        strip -S "${2}"/`basename "${r}"` || return 1
        if [ -L "${i}" ]; then
            ln -sf `basename "${r}"` "${2}"/`basename "${i}"` || return 1
        fi
    done

    return 0
}

macosx_one()
{
    local f
    local l
    local r
    local s
    local rb
    local sb

    s=`readlink -e "${1}"` || return 1
    sb=`basename "${s}"` || return 1

    if [ -f "${LIBDIR}/${sb}" ]; then
        return 0
    fi

    l=`otool -L "${s}" | tail -n +2 | awk "{
        if (index(\\$1, \"${ROOT}/\") == 1) {
            print \\$1
        }}"` || return 1

    for f in ${l}; do
        r=`readlink -e "${f}"` || return 1
        if [ x"${r}" = x"${s}" ]; then
            continue
        fi

        rb=`basename "${r}"` || return 1
        if [ -f "${LIBDIR}/${rb}" ]; then
            continue
        fi

        macosx_one "${r}" 1 || return 1
    done

    if [ 0 -eq "${2}" ]; then
        return 0
    fi

    cp -af "${s}" "${LIBDIR}/${sb}" || return 1
    strip -S "${LIBDIR}/${sb}" || return 1
    install_name_tool -id "${sb}" "${LIBDIR}/${sb}" || return 1
    return 0
}

macosx_two()
{
    local f
    local l
    local r
    local t
    local rb

    for f in "${LIBDIR}"/*.dylib; do
        if [ x"${f}" = x"${LIBDIR}/*.dylib" ]; then
            break
        fi

        l=`otool -L "${f}" | tail -n +2 | awk "{
            if (index(\\$1, \"${ROOT}/\") == 1) {
                print \\$1
            }}"` || return 1

        for t in ${l}; do
            r=`readlink -e "${t}"` || return 1
            rb=`basename "${r}"` || return 1
            install_name_tool -change "${t}" "@loader_path/${rb}" "${f}" || return 1
        done
    done

    return 0
}

macosx_three()
{
    local f
    local l
    local r
    local rb
    local newpath

    newpath=`echo | awk "{
        split(\\"${BINDIR}\\", b, \\"/\\")
        split(\\"${LIBDIR}\\", e, \\"/\\")
        size = length(b) < length(e) ? length(b) : length(e)
        for (i = 1; i < size; ++i) {
            if (b[i + 1] != e[i + 1]) {
                break
            }
        }

        remdir = i
        updir = length(b) - remdir
        ret = \\"@loader_path\\"
        for (i = 0; i < updir; ++i) {
            ret = ret \\"/..\\"
        }

        for (i = remdir + 1; i <= length(e); ++i) {
            if (e[i] == \\"\\") break
            ret = ret \\"/\\" e[i]
        }

        print ret
        exit 170
    }"`

    if [ $? -ne 170 ]; then
        return 1
    fi

    l=`otool -L "${BINNAME}" | tail -n +2 | awk "{
        if (index(\\$1, \"${ROOT}/\") == 1) {
            print \\$1
        }}"` || return 1

    for f in ${l}; do
        r=`readlink -e "${f}"` || continue
        rb=`basename "${r}"` || return 1
        install_name_tool -change "${f}" "${newpath}/${rb}" "${BINNAME}" || return 1
    done

    return 0
}

macosx()
{
    macosx_one "${BINNAME}" 0 || return 1
    macosx_two || return 1
    macosx_three || return 1
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
