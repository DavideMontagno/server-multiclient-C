#!/bin/bash

if [ $# -ne 2 ]; then
	echo " Missing DirName e t" 1>&2
	exit 1
fi

if test "$1" = "--help" || test "$2" = "--help"; then
	echo " I parametri inseriti devono essere diversi da --help"
	exit 1
fi

exec 3<$1
if [ $? != 0 ]; then
	echo " $1 path not valid" 1>&2
	exit 1
fi
#controllo parametro t
re='^[0-9]+([.][0-9]+)?$'
if ! [[ $2 =~ $re ]] ; then
   echo " $2 t non valido" >&2; exit 1
fi

aux=$IFS
IFS+="=" 
bool=0
#leggo dir name
while read -u 3 param value; do
	if test "$param" = "DirName"; then
		path=$value
		bool=1
		break;
	fi
done;
IFS=$aux

if [ $bool -eq 0 ]; then
	echo "DirName not found in $1" 1>&2
	exit 1
fi

if [ $2 -eq 0 ]; then
	ls $path
else
	find $path -mindepth 1 -mmin +$2 -delete
fi
