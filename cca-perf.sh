#!/usr/bin/bash

# Load libs
source lib/cca-perf-lib.sh

# Start time
tic=$(date +%s)

# Set paths
basehome=${PWD}
cd ../..
ns3home=${PWD}
cd $basehome

# Default values
serverType='Edge'
rlcBufferPer=200
tcpTypeId1='TcpNewReno'
tcpTypeId2='TcpNewReno'
tcpTypeId3='UDP'
bitrateType1="CBR"
bitrateType2="CBR"
bitrateType3="CBR"
speed1=1
speed2=1
speed3=0
xUE1=20
xUE2=20
xUE3=80

simTime=70
iniTime=0
endTime=$simTime
ueN1=1
ueN2=0
ueN3=0
frequency=27.3e9
bandwidth=200e6
myscenario=0
AQM='None'
dataRate1=60
dataRate2=60
dataRate3=60
verbose=0
nobuild=0
randstart=0
tag='00'
useAI=0
useECN=0
folder_name="--"
aqm_enqueue="dummy"
aqm_dequeue="dummy"
rng=20
logging=0
print_outfolder=0
delete_txt=0
circlepath=1
serverDelay=4
CustomLoss=0
CustomLossLos=0
CustomLossNlos=0

helpFunction()
{
   echo ""
   echo "Usage:   $0 [-t <cca>] [-s <location>] [-r <percentage>]"
   echo "Example: $0 -t $tcpTypeId1 -s $serverType -r $rlcBufferPer"
   echo "         $0 --verbose --randstart -t $tcpTypeId1 -t $tcpTypeId2 -t $tcpTypeId3 -s $serverType -r $rlcBufferPer"
   echo "         $0 --verbose --randstart --protocol $tcpTypeId1 --protocol $tcpTypeId2 --protocol $tcpTypeId3 --serverType $serverType --rlcbuffer $rlcBufferPer"
   echo -e "\noptions:"
   echo -e "\t--verbose \tPrint status and which part were executed."
   echo -e "\t--randstart \tAdd a random init time up to 1 [s] to each UE."
   echo -e "\t--no_build \tDon't build the project (default off)."
   echo -e "\t-a, --useAI \tUse a Python defined AQM (which includes an AI one). Remember to call it alongside --enqueue and/or --dequeue."
   echo -e "\t--useECN \tEnable Explicit Congestion Notification (ECN) (default off)."
   echo -e "\t-l, --logging \tEnable logging (default off)."
   echo -e "\t--outdir-print\tPrint the name that the outfolder will have, it does not run a simulation."
   echo -e "\t--rm-txt\tDelete the .txt files after the simulation runs (it does graph before)."
   echo -e "\t-h, --help \tPrint this message."
   echo -e "\nflags:"
   echo -e "\t--serverDelay \tSet the server delay."
   echo -e "\t--simTime \tSet the simulation time."
   echo -e "\t-t, --protocol \t'TcpNewReno' or 'TcpBbr' or 'TcpCubic' or 'TcpHighSpeed' or 'TcpBic' or 'TcpLinuxReno' or 'UDP' or 'QUIC'."
   echo -e "\t-b, --bitrateType \t'WBT' (Web Traffic), 'VBR1' (60 FPS), 'VBR2' (120 FPS), 'VBR3' (30 FPS) or 'CBR' (CONSTANT)."
   echo -e "\t-s, --serverType \t'Remote' or 'Edge'."
   echo -e "\t-r, --rlcbuffer \tRLC Buffer Percentage 10 or 100."
   echo -e "\t-d, --datarate \tdataRate of each flow from the server in Mbps."
   echo -e "\t-n, --uenumber \tNumber of UE Type 1, 2 and 3."
   echo -e "\t-c, --scenario \tScenario 0 or 3."
   echo -e "\t-i, --initime \tInitial time in seconds."
   echo -e "\t-e, --endtime \tEnd time in seconds."
   echo -e "\t-v, --speed \tSpeed of the first UE."
   echo -e "\t-g, --tag \tSet the tag"
   echo -e "\t-o, --outdir \tSet the name of the outfolder/outdirectory where files will be saved, if not defined the name is set based on the parameters chosen."
   echo -e "\t-q, --enqueue \tEnqueue AQM to use (options available in cca-perf.py, default dummmy). Needs flag --useAI."
   echo -e "\t-y, --dequeue \tDequeue AQM to use (options available in cca-perf.py, default dummmy). Needs flag --useAI."
   echo -e "\t-z, --seed \tRNG Seed to use in the simulation."
   echo -e "\t-p, --path \tChoose type of path (options: circlepath)."
   echo -e "\t-x, --position \tInitial position of User per Group. User can belong to group 1, 2 or 3. Group corresponds to the order entered."
   echo -e "\t--CustomLoss \tCustom loss."
   echo -e "\t--CustomLossLos\tCustom Line of Sight loss."
   echo -e "\t--CustomLossNlos \tCustom Non Line of Sight loss."
   exit 1 # Exit script after printing help
}




t=1
b=1
x=1
v=1
n=1
dr=1

while getopts "lq:y:t:r:s:n:x:b:c:o:ai:e:g:d:z:-:v:h?m:p:" opt
do
   case "$opt" in
      - )
         case "$OPTARG" in
            verbose ) verbose=1;;
            no-build ) nobuild=1;;
            randstart ) randstart=1;;
            useAI ) useAI=1;;
            logging ) logging=1;;
            rm-txt ) delete_txt=1;;
            outdir-print ) print_outfolder=1;;
            serverDelay | serverdelay) serverDelay="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            simTime | simtime ) simTime="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            protocol ) multit="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));
                     if [ $t = 1 ]; then
                        tcpTypeId1=$multit;
                     elif [ $t = 2 ]; then
                        tcpTypeId2=$multit;
                     else
                        tcpTypeId3=$multit;
                     fi
                     t=$(($t + 1));
                     ;;
            bitrateType | bitratetype ) multit="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));
                     if [ $b = 1 ]; then
                        bitrateType1=$multit;
                     elif [ $b = 2 ]; then
                        bitrateType2=$multit;
                     else
                        bitrateType3=$multit;
                     fi
                     b=$(($b + 1));
                     ;;

            servertype | serverType ) serverType="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            rlcbuffer | rlcBuffer ) rlcBufferPer="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            datarate | dataRate ) multi="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));
                                 if [ $dr == 1 ]; then
                                    dataRate1=$multi;
                                 elif [ $dr == 2 ]; then
                                    dataRate2=$multi;
                                 else
                                    dataRate3=$multi;
                                 fi
                                 dr=$(($dr + 1));
                                 ;;
            uenumber | ueNumber ) multin="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));
                                 if [ $n == 1 ]; then
                                    ueN1=$multin;
                                 elif [ $n == 2 ]; then
                                    ueN2=$multin;
                                 else
                                    ueN3=$multin;
                                 fi
                                 n=$(($n + 1));
                                 ;;
            speed ) multispeed="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 )) ;
                     if [ $v == 1 ]; then
                        speed1=$multispeed;
                     elif [ $v == 2 ]; then
                        speed2=$multispeed;
                     else
                        speed3=$multispeed;
                     fi
                     v=$(($v + 1));
                     ;;
            scenario ) myscenario="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            initime | iniTime ) iniTime="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            endtime | endTime ) endTime="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            tag ) tag="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            outdir ) folder_name="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            enqueue ) aqm_enqueue="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            dequeue ) aqm_dequeue="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            seed ) rng="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            path ) temp="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));
                  if [ $temp == "circle" ]; then
                     circlepath=1;
                  else
                     circlepath=0;
                  fi
                  ;;
            position ) multipos="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 )) ;
                     if [ $x == 1 ]; then
                        xUE1=$multipos;
                     elif [ $x == 2 ]; then
                        xUE2=$multipos;
                     else
                        xUE3=$multipos;
                     fi
                     x=$(($x + 1));
                     ;;
            CustomLoss | customLoss | customloss ) CustomLoss="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            CustomLossLos | customLossLos | customlosslos ) CustomLossLos="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            CustomLossNlos | customLossNlos | customlossnlos ) CustomLossNlos="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            useecn | useECN ) useECN=1;;
            help ) helpFunction;;
            * );;
         esac
         ;;
      l ) logging=1;;
      q ) aqm_enqueue="$OPTARG";;
      y ) aqm_dequeue="$OPTARG";;
      t )
         multit=("$OPTARG") ;
         if [ $t = 1 ]; then
            tcpTypeId1=$multit;
         elif [ $t = 2 ]; then
            tcpTypeId2=$multit;
         else
            tcpTypeId3=$multit;
         fi
         t=$(($t + 1));
         ;;
      x )
         multix=("$OPTARG") ;
         if [ $x == 1 ]; then
            xUE1=$multix;
         elif [ $x == 2 ]; then
            xUE2=$multix;
         else
            xUE3=$multix;
         fi
         x=$(($x + 1));
         ;;
      b )
         multit=("$OPTARG") ;
         if [ $b = 1 ]; then
            bitrateType1=$multit;
         elif [ $b = 2 ]; then
            bitrateType2=$multit;
         else
            bitrateType3=$multit;
         fi
         b=$(($b + 1));
         ;;
      v )
         multispeed=("$OPTARG") ;
         if [ $v == 1 ]; then
            speed1=$multispeed;
         elif [ $v == 2 ]; then
            speed2=$multispeed;
         else
            speed3=$multispeed;
         fi
         v=$(($v + 1));
         ;;
      n )
         multin=("$OPTARG") ;
         if [ $n == 1 ]; then
            ueN1=$multin;
         elif [ $n == 2 ]; then
            ueN2=$multin;
         else
            ueN3=$multin;
         fi
         n=$(($n + 1));
         ;;
      s ) serverType="$OPTARG" ;;
      r ) rlcBufferPer="$OPTARG" ;;
      d ) multi="$OPTARG" ;
         if [ $dr == 1 ]; then
            dataRate1=$multi;
         elif [ $dr == 2 ]; then
            dataRate2=$multi;
         else
            dataRate3=$multi;
         fi
         dr=$(($dr + 1));
         ;;
      c ) myscenario="$OPTARG" ;;
      a ) useAI=1 ;;
      i ) iniTime="$OPTARG" ;;
      e ) endTime="$OPTARG" ;;
      g ) tag="$OPTARG" ;;
      o ) folder_name="$OPTARG" ;;
      z ) rng="$OPTARG" ;;
      m ) ;;
      p ) temp="$OPTARG" ;
         if [ $temp == "circle" ]; then
            circlepath=1;
         else
            circlepath=0;
         fi
         ;;
      h ) helpFunction ;;
      ? ) helpFunction ;; # Print helpFunction in case parameter is non-existent
   esac
done

##########################################################
## CHECK INTEGRITY OF THE ARGUMENTS
if [ "$verbose" != "0" ] && [ "$verbose" != "1" ]; then
   echo "Verbose \"$verbose\" no available";
   helpFunction
fi

if [ "$useECN" != "0" ] && [ "$useECN" != "1" ]; then
   echo "useECN \"$verbose\" not available";
   helpFunction
fi

if [ "$nobuild" != "0" ] && [ "$nobuild" != "1" ]; then
   echo "No Build \"$nobuild\" no available";
   helpFunction
fi

if [ "$useAI" != "0" ] && [ "$useAI" != "1" ]; then
   echo "useAI \"$useAI\" not a valid option";
   helpFunction
fi

# If both AQM are dummy, then do not use python to execute the experiment
if [ "$aqm_enqueue" == "dummy" ] && [ "$aqm_dequeue" == "dummy" ]; then
   useAI=0

   if [ "$print_outfolder" == "0" ]; then
      echo "Use AI option disabled, both AQM were dummy.";
   fi

fi

if [ "$myscenario" != "0" ] && [ "$myscenario" != "3" ]; then
   echo "Scenario \"$myscenario\" no available";
   helpFunction
fi

if [ "$serverType" != "Remote" ] && [ "$serverType" != "Edge" ]; then
   echo "ServerType \"$serverType\" no available";
   helpFunction
fi

if [ $rlcBufferPer -gt 99999 ] || [ $rlcBufferPer -lt 1 ]; then
   echo "rlcBuffer \"$rlcBufferPer\" no available";
   helpFunction
fi

if [ $ueN1 -gt 20 ] || [ $ueN1 -lt 1 ]; then
   echo "UE \"$ueN1\" no available";
   helpFunction
fi

if [ $ueN2 -gt 20 ] || [ $ueN2 -lt 0 ]; then
   echo "UE \"$ueN2\" no available";
   helpFunction
fi

if [ $ueN3 -gt 20 ] || [ $ueN3 -lt 0 ]; then
   echo "UE \"$ueN3\" no available";
   helpFunction
fi
if [ $iniTime -gt $simTime ] || [ $iniTime -lt 0 ]; then
   echo "Init time \"$iniTime\" between 0-\"$simTime\". ";
   helpFunction
fi

if [ $endTime -gt $simTime ] ; then
   endTime=$simTime
fi
if [ $endTime -gt $simTime ] || [ $endTime -lt $iniTime ]; then
   echo "End time \"$endTime\" between $iniTime-$simTime. ";
   helpFunction
fi

if [ $CustomLoss -lt 0 ]; then
   echo "CustomLoss \"$CustomLoss\" no available";
   helpFunction
fi

if [ $CustomLossLos -lt 0 ]; then
   echo "CustomLossLos \"$CustomLossLos\" no available";
   helpFunction
fi

if [ $CustomLossNlos -lt 0 ]; then
   echo "CustomLossNlos \"$CustomLossNlos\" no available";
   helpFunction
fi

if [ "$tcpTypeId1" != "QUIC" ] && [ "$tcpTypeId1" != "UDP" ] && [ "$tcpTypeId1" != "TcpNewReno" ] && [ "$tcpTypeId1" != "TcpBbr" ] && [ "$tcpTypeId1" != "TcpCubic" ] && [ "$tcpTypeId1" != "TcpHighSpeed" ] && [ "$tcpTypeId1" != "TcpBic" ] && [ "$tcpTypeId1" != "TcpLinuxReno" ] && [ "$tcpTypeId1" != "TcpDctcp" ]
then
   echo "tcpTypeId1 \"$tcpTypeId\" no available";
   helpFunction
fi

##########################################################
## CREATE DIRECTORIES
servertag=${serverType:0:1}
delaytag=$(printf "%04d" ${serverDelay})
buffertag=$(printf "%03d" ${rlcBufferPer})
cltag=$(printf "%02d" ${CustomLoss})
cllostag=$(printf "%02d" ${CustomLossLos})
clnlostag=$(printf "%02d" ${CustomLossNlos})

myhome=$basehome
me=`basename "$0"`
myapp=`echo "$me" | cut -d'.' -f1`

#create out folder
outfolder="${myhome}/out"
if [ ! -d "$outfolder" ];
then
	mkdir $outfolder
fi

#create bk folder
if [ "$folder_name" == "--" ];
then
   # bkfolder=${myscenario}"-"$tcpTypeId1"-"$tcpTypeId2"-"$tcpTypeId3"-"$servertag$delaytag"-"$buffertag"-UE_"${ueN1}"_"${ueN2}"_"${ueN3}"-UseAI_"${useAI}-EQ_${aqm_enqueue}-DQ_${aqm_dequeue}"-"${iniTime}_${endTime}"-"${dataRate}"-"${cltag}"-"${cllostag}"-"${clnlostag}"-"${tag}
   bkfolder=$tcpTypeId1"-"$tcpTypeId2"-"$tcpTypeId3"-"$buffertag"-UE_"${ueN1}"_"${ueN2}"_"${ueN3}"-UseAI_"${useAI}-EQ_${aqm_enqueue}-DQ_${aqm_dequeue}"-"${iniTime}_${endTime}"-"${dataRate1}"_"${dataRate2}"_"${dataRate3}"-"${cltag}"-"${cllostag}"-"${clnlostag}"-"${tag}
else
   bkfolder=$folder_name
fi

# Print the outfolder only
if [ "$print_outfolder" == "1" ];then
   echo $bkfolder
   exit 0
fi

if [ ! -d "$outfolder/$bkfolder" ];then
	mkdir $outfolder/$bkfolder
else
   rm $outfolder/$bkfolder/*
fi

##########################################################
# BUILD NS3 AND RUN SIMULATIONS

if [ "$nobuild" == "0" ]
then 
   echo "Building ns3...";
   "${ns3home}/ns3" build
   
   if [ "$?" != "0" ]; then
      printf "${red}Error while building, simulation cancelled! ${clear}\n"
      exit 1
   fi

   # clear
   echo "NS3 was built!"
fi

options="`if [ $verbose -eq 1 ]; then echo "--verbose"; fi` `if [ $nobuild -eq 1 ]; then echo "--no-build"; fi`"
printf "\nRunning ${TXT_MAGENTA}$0 ${options} --serverDelay ${serverDelay} -t ${tcpTypeId1} -t ${tcpTypeId2} -t ${tcpTypeId3} -r ${rlcBufferPer} -s ${serverType} -i ${iniTime} -e ${endTime} -n ${ueN1} -n ${ueN2} -n ${ueN3} -x ${xUE1} -x ${xUE2} -x ${xUE3} -c ${myscenario} -a ${useAI} -q ${aqm_enqueue} -y ${aqm_dequeue} -d ${dataRate1} -d ${dataRate2} -d ${dataRate3} -g ${tag}${TXT_CLEAR}\n"

printf "\ttcpTypeId: ${TXT_MAGENTA}${tcpTypeId1}-${tcpTypeId2}-${tcpTypeId3}${TXT_CLEAR}\n"
printf "\t#UEs:      ${TXT_MAGENTA}${ueN1}-${ueN2}-${ueN3}${TXT_CLEAR}\n"
printf "\tspeed:     ${TXT_MAGENTA}${speed1}-${speed2}-${speed3}${TXT_CLEAR}\n"
printf "\tRLCBuffer: ${TXT_GREEN}${rlcBufferPer}${TXT_CLEAR}\n"
printf "\tServer:    ${TXT_GREEN}${serverType}\t${serverDelay}${TXT_CLEAR}\n"
printf "\tScenario:  ${TXT_GREEN}${myscenario}${TXT_CLEAR}\n"
printf "\tFrequency: ${TXT_GREEN}${frequency}${TXT_CLEAR}\n"
printf "\tBandwidth: ${TXT_GREEN}${bandwidth}${TXT_CLEAR}\n"
printf "\tData Rate:  ${TXT_GREEN}${dataRate1}-${dataRate2}-${dataRate3}${TXT_CLEAR}\n"
printf "\tCustom Loss     :  ${TXT_GREEN}${CustomLoss}${TXT_CLEAR}\n"
printf "\tCustom Loss LoS :  ${TXT_GREEN}${CustomLossLos}${TXT_CLEAR}\n"
printf "\tCustom Loss NLoS:  ${TXT_GREEN}${CustomLossNlos}${TXT_CLEAR}\n"

echo

echo $outfolder/$bkfolder

# Flag for "still running"
touch $outfolder/$bkfolder/.still_running

#backup run-sim and cc
cp $me $outfolder/$bkfolder/$me.txt
cp ${myhome}/${myapp}.cc $outfolder/$bkfolder/${myapp}.cc.txt
cp packet-error-rate.sh $outfolder/$bkfolder/packet-error-rate.sh.txt
cp cca-perf-graph.py $outfolder/$bkfolder/cca-perf-graph.py.txt

if [ "$useAI" == "0" ]; then
   $ns3home/ns3 run "`echo ccaperf-exec`
      --frequency=`echo $frequency`
      --bandwidth=`echo $bandwidth`
      --tcpTypeId1=`echo $tcpTypeId1`
      --tcpTypeId2=`echo $tcpTypeId2`
      --tcpTypeId3=`echo $tcpTypeId3`
      --bitrateType1=`echo $bitrateType1`
      --bitrateType2=`echo $bitrateType2`
      --bitrateType3=`echo $bitrateType3`
      --serverType=`echo $serverType`
      --serverDelay=`echo $serverDelay`
      --ueN1=`echo $ueN1`
      --ueN2=`echo $ueN2`
      --ueN3=`echo $ueN3`
      --xUE1=`echo $xUE1`
      --xUE2=`echo $xUE2`
      --xUE3=`echo $xUE3`
      --speed1=`echo $speed1`
      --speed2=`echo $speed2`
      --speed3=`echo $speed3`
      --randstart=`echo $randstart`
      --rlcBufferPerc=`echo $rlcBufferPer`
      --dataRate1=`echo $dataRate1`
      --dataRate2=`echo $dataRate2`
      --dataRate3=`echo $dataRate3`
      --simTime=`echo $simTime`
      --iniTime=`echo $iniTime`
      --endTime=`echo $endTime`
      --phyDistro=`echo $myscenario`
      --useAI=`echo $useAI`
      --useECN=`echo $useECN`
      --verbose=`echo $verbose`
      --rng=`echo $rng`
      --logging=`echo $logging`
      --circlepath=`echo $circlepath`
      --CustomLoss=`echo $CustomLoss`
      --CustomLossLos=`echo $CustomLossLos`
      --CustomLossNlos=`echo $CustomLossNlos`
      " --cwd "$outfolder/$bkfolder" --no-build
else
   # $ns3home/ns3 run "`echo ccaperf-exec`
   python3 cca-perf.py\
      --frequency=`echo $frequency`\
      --bandwidth=`echo $bandwidth`\
      --tcpTypeId1=`echo $tcpTypeId1`\
      --tcpTypeId2=`echo $tcpTypeId2`\
      --tcpTypeId3=`echo $tcpTypeId3`\
      --bitrateType1=`echo $bitrateType1`\
      --bitrateType2=`echo $bitrateType2`\
      --bitrateType3=`echo $bitrateType3`\
      --serverType=`echo $serverType`\
      --serverDelay=`echo $serverDelay`\
      --ueN1=`echo $ueN1`\
      --ueN2=`echo $ueN2`\
      --ueN3=`echo $ueN3`\
      --xUE1=`echo $xUE1`\
      --xUE2=`echo $xUE2`\
      --xUE3=`echo $xUE3`\
      --speed1=`echo $speed1`\
      --speed2=`echo $speed2`\
      --speed3=`echo $speed3`\
      --randstart=`echo $randstart`\
      --rlcBufferPerc=`echo $rlcBufferPer`\
      --dataRate1=`echo $dataRate1`\
      --dataRate2=`echo $dataRate2`\
      --dataRate3=`echo $dataRate3`\
      --simTime=`echo $simTime`\
      --iniTime=`echo $iniTime`\
      --endTime=`echo $endTime`\
      --phyDistro=`echo $myscenario`\
      --useAI=`echo $useAI`\
      --useECN=`echo $useECN`\
      --verbose=`echo $verbose`\
      --enqueue `echo $aqm_enqueue`\
      --dequeue `echo $aqm_dequeue`\
      --rng=`echo $rng`\
      --logging=`echo $logging`\
      --circlepath=`echo $circlepath`\
      --CustomLoss=`echo $CustomLoss`\
      --CustomLossLos=`echo $CustomLossLos`\
      --CustomLossNlos=`echo $CustomLossNlos`\
      --cwd "$outfolder/$bkfolder" # It never builds (!)
fi

exit_status=$?

# In case the simulation was aborted, exit and print code error.
if [ "$exit_status" != "0" ]; then
   printf "${TXT_RED}Error ${exit_status} while simulating, simulation was aborted! ${TXT_CLEAR}\n"
   echo "Graphing, per calculation and backup were not run. Compressing..."
   gzip $outfolder/$bkfolder/*.txt
   exit $exit_status
fi

##########################################################
# POST PROCESS THE DATA ACCUMULATED IN CASE OF SUCCESS

echo
printf "Running... Packet Error Rate Script\n"
echo

./packet-error-rate.sh $outfolder/$bkfolder


echo
printf "Running... Graph Script\n"
echo

echo "python3 cca-perf-graph.py $outfolder/$bkfolder"
python3 cca-perf-graph.py $outfolder/$bkfolder

if [ "$delete_txt" == "1" ]; then
   echo
   printf "Removing .txt files\n"
   rm $outfolder/$bkfolder/*.txt
fi

echo
printf "Compressing files\n"
echo
gzip $outfolder/$bkfolder/*.txt &
gzip $outfolder/$bkfolder/*.csv &

__full_path=$outfolder/$bkfolder
source cca-sim-backup.sh $outfolder/$bkfolder

toc=$(date +%s)
printf "Simulation Processed in: "${TXT_MAGENTA}$(($toc-$tic))${TXT_CLEAR}" seconds\n"
rm $__full_path/.still_running