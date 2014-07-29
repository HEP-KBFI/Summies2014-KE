#!/bin/bash

# A POSIX variable
# Reset in case getopts has been used previously in the shell
OPTIND=1

# Initialize our variables:
input=
out=
dir=
ext=
dimx=
dimy=

function usage {
cat << EOF
Usage: $0 options

This script generates plots from the input *.root files.

OPTIONS:
   -I <input>      Input *.root files
   -o <output>     Output *.root file name
   [-d <dir>]      Directory of plots
   -e <extension>  File extension of the plots
   [-x <int>]      x-dimension of the plots
   [-y <int>]      y-dimension of the plots
EOF
}

while getopts ":I:o:d:e:x:y:" opt; do
	case "$opt" in
	I)  input=$OPTARG
		;;
	o)  out=$OPTARG
		;;
	d)  dir=$OPTARG
		;;
	e)  ext=$OPTARG
		;;
	x)  dimx=$OPTARG
		;;
	y)  dimy=$OPTARG
		;;
	*)
		usage
		exit 0
	esac
done

if [[ -z $input ]] || [[ -z $out ]] || [[ -z $ext ]]; then
	usage
	exit 1
fi

dimx_flag=
dimy_flag=

if [[ ! -z $dimx ]]; then
	dimx_flag="-x $dimx"
fi

if [[ ! -z $dimy ]]; then
	dimy_flag="-y $dimy"
fi

dir_flag=

if [[ ! -z $dir ]]; then
	if [[ ! -d "$dir" ]]; then
		mkdir $dir
	fi
	dir_flag="-d $dir"
fi

# add the histograms
hadd $out.root $input*.root

# create plots
./histoplot.out -I $out.root $dimx_flag $dimy_flag -e $ext $dir_flag

# EOF
