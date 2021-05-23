#!/bin/sh
#
# Sample script to run make without having to retype the long path each time
# This will work if you built the environment using our ~/bin/build-snap script

PROJECT_NAME=eventdispatcher
PROJECT_DIR=../../BUILD/Debug/contrib/${PROJECT_NAME}
PROCESSORS=`nproc`

case $1 in
"-l")
    make -C ${PROJECT_DIR} 2>&1 | less -SR
    ;;

"-d")
    rm -rf ${PROJECT_DIR}/doc/${PROJECT_NAME}-doc-1.0.tar.gz
    make -C ${PROJECT_DIR}
    ;;

"-i")
    make -j${PROCESSORS} -C ${PROJECT_DIR} install
    ;;

"-t")
    (
        if make -j${PROCESSORS} -C ${PROJECT_DIR}
        then
            shift
            ${PROJECT_DIR}/tests/unittest --progress $*
        fi
    ) 2>&1 | less -SR
    ;;

"")
    make -j${PROCESSORS} -C ${PROJECT_DIR}
    ;;

*)
    echo "error: unknown command line option \"$1\""
    ;;

esac

# vim: ts=4 sw=4 et
