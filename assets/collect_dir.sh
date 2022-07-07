#!/bin/bash

collect_dir() {
    echo $1
    subdirs=`cat $1/dir_list`
    for dir in $subdirs; do
        collect_dir $1/$dir
    done
}

TOP_DIR=`echo $1 | sed "s|/$||"`
collect_dir $TOP_DIR