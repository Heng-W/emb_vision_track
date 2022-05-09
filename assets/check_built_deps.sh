#!/bin/bash

#echo $1
for dep in $1; do
    if [ -f $dep ]; then
        file_list=$(paste -s $dep | cut -d':' -f 2 | sed 's/\\/ /g')
        #echo $file_list
        for file in $file_list; do
            if [ ! -f $file ]; then rm $dep; break; fi
        done
    fi
done

