#!/bin/sh
# Usage: makedist.sh <binary dir> <dist dir> <zip dist>

BIN_DIR=`echo $1 | sed 's/ /\\\\ /g'`
DIST_DIR=`echo $2 | sed 's/ /\\\\ /g'`
DIST_BASE=`eval basename ${DIST_DIR}`
DIST_BASE=`echo ${DIST_BASE} | sed 's/ /\\\\ /g'`
ZIP_DIST=`echo $3 | sed 's/ /\\\\ /g'`

eval mkdir -p ${DIST_DIR}

# Copy binary.
BINARY_IN=${BIN_DIR}/efdl
eval cp ${BINARY_IN} ${DIST_DIR}
BINARY_OUT=${DIST_DIR}/efdl

# Copy Qt libraries.
if [ "`uname`" = "Darwin" ]; then
    for frmw in `otool -L ${BINARY_OUT} | grep -ie "qt.*\.framework" | xargs -I{} echo "{}" | awk '{print $1}'`; do
        cp ${frmw} ${DIST_DIR}
        name=`echo ${frmw} | sed -E 's/(.+)([Qq][Tt].+)(\.[Ff][Rr][Aa][Mm][Ee][Ww][Oo][Rr][Kk].*)/\2/'`
        install_name_tool -id "@executable_path/${name}" ${DIST_DIR}/${name}
        install_name_tool -change ${frmw} "@executable_path/${name}" ${BINARY_OUT}
        if [ "${name}" == "QtNetwork" ]; then
            core=`otool -L ${DIST_DIR}/${name} | grep -ie "qtcore.*\.framework" | xargs -I{} echo "{}" | awk '{print $1}'`
            install_name_tool -change ${core} "@executable_path/QtCore" ${DIST_DIR}/${name}
        fi
    done
fi

# Create zip file.
eval echo "Creating zip file: ${ZIP_DIST}"
cd ${BIN_DIR}
rm -f dist.zip
eval zip -r9q dist.zip ${DIST_BASE}
eval mv dist.zip ${ZIP_DIST}

echo "Cleaning up"
eval rm -fr ${DIST_DIR}
