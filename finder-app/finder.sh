#!/bin/bash
filesdir=$1
searchstr=$2

if [ "$#" -ne 2 ]
	then
		echo "please check the arguments"
		exit 1

fi

if [ -d "${filesdir}" ]
	then
		X=`find ${filesdir} -type f | wc -l`
		Y=`grep -r ${searchstr} ${filesdir} | wc -l`
		echo "The number of files are ${X} and the number of matching lines are ${Y}"
		exit 0
	else
		echo "file directory unavailable"
		exit 1
fi
