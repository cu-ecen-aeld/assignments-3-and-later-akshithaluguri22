#!/bin/bash
writefile=$1
writestr=$2

if [ "$#" -ne 2 ]
	then
		echo "please check the arguments"
		exit 1
fi

if ! [ -d "$(dirname $writefile)" ]
	then
		`mkdir -p "$(dirname $writefile)"`
fi

if touch $writefile
	then
		echo "$writestr" > $writefile
		exit 0
	else
		echo "file could not be created"
		exit 1
fi
