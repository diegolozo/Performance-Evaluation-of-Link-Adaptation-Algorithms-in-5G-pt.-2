#!/usr/bin/bash
tic=$(date +%s)
source lib/cca-perf-lib.sh

if [ "$#" -eq 1 ]; then
   if [ -d "$1" ]; then
      printf "Starting to backup folder ${TXT_MAGENTA}$1${TXT_CLEAR}\n"
   else
      printf "Folder ${TXT_RED}$1${TXT_CLEAR} does not exist.\n"
      exit
   fi
else
   echo "Bye"
   exit
fi

orifolder=$1

if [ "${orifolder:0:2}" = "./" ]; then
   orifolder=${orifolder:2}
fi

bkfolder="s3://jsandova-uchile-cl/2024-sim/ns-3.41/scratch/cca-perf/${orifolder}/"

printf "From "${orifolder}" to "${bkfolder}"\n"

s3cmd put ${orifolder}/cca-perf.*  $bkfolder
s3cmd put ${orifolder}/*.ini  $bkfolder
s3cmd put ${orifolder}/*.png  $bkfolder
s3cmd put ${orifolder}/*.json  $bkfolder
s3cmd put ${orifolder}/output.log  $bkfolder

toc=$(date +%s)
printf "Backup Processed in: "${TXT_MAGENTA}$(($toc-$tic))${TXT_CLEAR}" seconds\n"

