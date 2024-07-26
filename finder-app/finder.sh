#!/bin/sh
if [ $# -ne 2 ]
then
    echo "Script expects 2 string arguments 1->path, 2->string to search" 
    exit 1;
else    
    filesdir=$1;
    searchstr=$2;
    if ! [ -d ${filesdir} ]
    then
        echo "Directroy not found ${filesdir}"
        exit 1;
    else
        X=$(find ${filesdir} -type f | wc -l);
        Y=$(grep -r ${searchstr} ${filesdir} | wc -l);
        echo "The number of files are $X and the number of matching lines are $Y"
        exit 0;
    fi
fi
