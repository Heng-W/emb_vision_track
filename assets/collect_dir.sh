#!/bin/bash

collect_dir(){
    subdir_list=`cat $1/dir_list`
    for dir in $subdir_list; do
        echo $1/$dir
        collect_dir $1/$dir
    done
}

collect_dir $1