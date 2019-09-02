#!/bin/sh
#
# Sample script to run make without having to retype the long path each time
# This will work if you built the environment using our ~/bin/build-snap script

PROJECT_NAME=eventdispatcher
PROCESSORS=4

case $1 in
"-l")
    make -C ../../../BUILD/contrib/${PROJECT_NAME} 2>&1 | less -SR
    ;;

"-d")
    rm -rf ../../../BUILD/contrib//${PROJECT_NAME}/doc//${PROJECT_NAME}-doc-1.0.tar.gz
    make -C ../../../BUILD/contrib//${PROJECT_NAME}
    ;;

"-i")
    make -j${PROCESSORS} -C ../../../BUILD/contrib//${PROJECT_NAME} install
    ;;

"-t")
    (
        if make -j${PROCESSORS} -C ../../../BUILD/contrib//${PROJECT_NAME}
        then
            shift
            ../../../BUILD/contrib//${PROJECT_NAME}/tests/unittest --progress $*
        fi
    ) 2>&1 | less -SR
    ;;

"")
    make -j${PROCESSORS} -C ../../../BUILD/contrib//${PROJECT_NAME}
    ;;

*)
    echo "error: unknown command line option \"$1\""
    ;;

esac

# vim: ts=4 sw=4 et
