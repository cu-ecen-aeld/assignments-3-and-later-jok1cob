#!/bin/bash
if [ $# -ne 2 ]
then
    echo "Script expects 2 string arguments 1->path, 2->string to write" 
    exit 1;
else    
    writefile=$1;
    writestr=$2;
    if ! [ -d $(dirname ${writefile}) ]
    then    
        mkdir -p "$(dirname ${writefile})"
    fi
    touch ${writefile}
    if ! [ $? -eq 0 ]
    then
        echo "Error cannot create file"
        exit 1;
    else
        echo ${writestr} > ${writefile}
        exit 0;
    fi
fi