#! /bin/sh

URL="."
if [ $# -eq 1 ]; then
    URL="${1}"
elif [ $# -gt 1 ]; then
    echo "${0} [project path]"
    exit 1
fi

FILE="revision.c"
Result="1"

export LC_ALL="en_US.UTF-8"

svn info ${URL} >/dev/null 2>&1
if [ "$?" -eq 0 ]; then
    Result="0"

    VCS_NAME="\"Subversion\""
    VCS_PROJECT_URL="\"`svn info ${URL} | tail -10 | head -1 | awk '{print $NF}'`\""
    VCS_PROJECT_ROOT="\"`svn info ${URL} | tail -9 | head -1 | awk '{print $NF}'`\""
    VCS_PROJECT_REVISION="`svn info ${URL} | tail -3 | head -1 | awk '{print $NF}'`"

    VCS_TIMESTAMP="`svn info ${URL} | tail -2 | head -1 | awk 'BEGIN{FS=\":\"} {print $2\":\"$3\":\"$4}' | awk '{print $1 \" \" $2}'`"
    if [ "`uname -o`" = "GNU/Linux" ] || [ "`uname -o`" = "Darwin" ]; then
        VCS_PROJECT_TIMESTAMP="`date -d \"${VCS_TIMESTAMP}\" +%s`"
    else
        VCS_PROJECT_TIMESTAMP="`date -j -f \"%Y-%m-%e %H:%M:%S\" \"${VCS_TIMESTAMP}\" +%s`"
    fi
fi

if [ "${Result}" -ne 0 ]; then
    VCS_NAME="NULL"
    VCS_PROJECT_URL="NULL"
    VCS_PROJECT_ROOT="NULL"
    VCS_PROJECT_REVISION="-1"
    VCS_PROJECT_TIMESTAMP="-1"
fi

if [ -n "${SCMPRODUCT}" ]; then
    VCS_PROJECT_VERSION="\"${SCMPRODUCT}\""
else
    VCS_PROJECT_VERSION='NULL'
fi

if [ -r "${FILE}" ]; then
    NOW_REV="`grep VCS_PROJECT_REVISION ${FILE} | awk '{print $5}'`"
    NOW_VER="`grep VCS_PROJECT_VERSION ${FILE} | awk '{print $6}'`"
    if [ x"${NOW_REV}" = x"${VCS_PROJECT_REVISION}" ] && [ x"${NOW_VER}" = x"${VCS_PROJECT_VERSION}" ]; then
        exit 0
    fi
fi

VCS_BUILD_HOSTNAME=\"`hostname -f`\"
VCS_BUILD_TIMESTAMP=`date +%s`

echo "   GEN     ${FILE}"
echo "#include <time.h>"                                                     > ${FILE}
echo                                                                        >> ${FILE}
echo "const char *const VCS_NAME              = ${VCS_NAME} ;"              >> ${FILE}
echo "const char *const VCS_PROJECT_URL       = ${VCS_PROJECT_URL} ;"       >> ${FILE}
echo "const char *const VCS_PROJECT_ROOT      = ${VCS_PROJECT_ROOT} ;"      >> ${FILE}
echo "const         int VCS_PROJECT_REVISION  = ${VCS_PROJECT_REVISION} ;"  >> ${FILE}
echo "const      time_t VCS_PROJECT_TIMESTAMP = ${VCS_PROJECT_TIMESTAMP} ;" >> ${FILE}
echo "const char *const VCS_PROJECT_VERSION   = ${VCS_PROJECT_VERSION} ;"   >> ${FILE}
echo "const char *const VCS_BUILD_HOSTNAME    = ${VCS_BUILD_HOSTNAME} ;"    >> ${FILE}
echo "const      time_t VCS_BUILD_TIMESTAMP   = ${VCS_BUILD_TIMESTAMP} ;"   >> ${FILE}
exit 0
