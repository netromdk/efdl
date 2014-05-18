#!/bin/sh
# Usage: makedist.sh <binary dir> <dist dir> <zip dist>

BIN_DIR=`echo $1 | sed 's/ /\\\\ /g'`
DIST_DIR=`echo $2 | sed 's/ /\\\\ /g'`
DIST_BASE=`eval basename ${DIST_DIR}`
DIST_BASE=`echo ${DIST_BASE} | sed 's/ /\\\\ /g'`
ZIP_DIST=`echo $3 | sed 's/ /\\\\ /g'`

eval mkdir -p ${DIST_DIR}

# Copy binary
eval cp ${BIN_DIR}/efdl ${DIST_DIR}

# Create zip file.
eval echo "Creating zip file: ${ZIP_DIST}"
cd ${BIN_DIR}
rm -f dist.zip
eval zip -r9q dist.zip ${DIST_BASE}
eval mv dist.zip ${ZIP_DIST}

echo "Cleaning up"
eval rm -fr ${DIST_DIR}
