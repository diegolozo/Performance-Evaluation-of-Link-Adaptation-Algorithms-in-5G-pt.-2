#!/usr/bin/bash

# Load libs
source lib/cca-perf-lib.sh

# Start time
tic=$(date +%s)

# Set paths
basehome=${PWD}
outfolder="./out"

myfolders=$(find $outfolder -type f -name mobilityPosition.txt* | sed 's:[^/]*$::')


max_proc=`lscpu | grep '^CPU(s)' | cut -d':' -f2`
max_proc=`expr ${max_proc} / 2`

printf "%s\n" "${myfolders[@]}" | xargs --max-procs=${max_proc} -I % python3 cca-perf-graph.py %


toc=$(date +%s)
printf "\nProcessed all Folders in: "${magenta}$(($toc-$tic))${clear}" seconds\n"