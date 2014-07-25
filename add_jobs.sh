#!/bin/bash

# A POSIX variable
# Reset in case getopts has been used previously in the shell
OPTIND=1

# Initialize our own variables:
begin=1
end=
prefix=

function usage {
cat << EOF
Usage: $0 options

This script submits *.sh files of a given pattern to SLURM.

OPTIONS:
   -p <prefix>     Prefix of the name of the script (e.g. <prefix>*.sh)
   [-b <int>]      Number of the first script
   -e <int>        Number of the last script
EOF
}

while getopts ":p:b:e:" opt; do
	case "$opt" in
	p)  prefix=$OPTARG
		;;
	b)  begin=$OPTARG
		;;
	e)  end=$OPTARG
		;;
	*)
		usage
		exit 0
	esac
done

if [[ -z $prefix ]] || [[ -z $end ]]; then
	usage
	exit 1
fi

if [ "$begin" -ge "$end" ]; then
	echo -n "error: Number of the last script cannot be smaller "
	echo "than the number of the first script." 1>&2
	exit 1
fi

for i in $(seq $begin $end); do
	if [ ! -f $prefix$i".sh" ]; then
		echo "error: the file $prefix$i.sh does not exist" 1>&2
		echo "no jobs were added"
		exit 1
	fi
done

for i in $(seq $begin $end); do
	sbatch $prefix$i".sh"
done

# EOF
