#!/bin/bash

# Custom vars
# Set paths
basehome=${PWD}
cd ../..
ns3home=${PWD}
cd $basehome
os=$(uname)
source lib/cca-perf-lib.sh
# Parallelize simulations
# parameters=("--verbose --no-build --randstart -c 0  -t UDP  -t UDP  -t UDP -i 0 -e 60 -n 2 -n 0 -n 0 -r 10 -x 20   -x 20    -x 80   -v 1  -v 1  -v 0 -d 600 -g 00 -m 22"
#             "--verbose  --no-build --randstart -t TcpBbr  -t TcpNewReno  -t UDP -n 1 -n 1 -n 0 -r 200 --useAI -q ARED-0.9  -x 20    -x 20    -x 80   -v 1  -v 1  -v 0 -d 600 -c 3")

# Lets remove whitelines from the parameters file, just in case
sed -i '/^\s*$/d' parameters.ccaperf
source parameters.ccaperf
_cmds_to_run=()

montecarlo=2
# Num of cores will be the number of parallel simulations
num_cores=$(($(nproc) / 2))

outdir=""
outpath==""
sim_num=1
nparam=0
custom_name=""
random=1
build_ns3=1

helpFunction()
{
   echo ""
   echo "Usage: $0 [-m $montecarlo] [-c OutDirectory] [-r] [-b] [-n ${num_cores}]"
   echo "       $0 [--montecarlo $montecarlo] [--outDir OutDirectory] [--notRandom] [--noBuild] [--maxSim ${num_cores}]"
   echo -e "\nflags:"
   echo -e "\t-m, --montecarlo \tNumber of montecarlo iterations [default: ${montecarlo}]."
   echo -e "\t-c, --outDir \tCustom folder/directory name for the folder with the different simulations, the word \"PAR-\" will be prefixed."
   echo -e "\t-r, --notRandom \tDeactivate the use of different rng seeds per montercarlo iteration."
   echo -e "\t-b, --noBuild \tSkips the build step of ns3, it always build by default."
   echo -e "\t-n, --maxSim \tMaximum number of simultaneous simulations [default: ${num_cores}]."
   echo -e "\t-h, --help \tPrint this message."
   exit 1 # Exit script after printing help
}

while getopts "m:c:rbn:-:h?" opt
do
   case "$opt" in
      - )
         case "$OPTARG" in
            montecarlo ) montecarlo="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            outDir | outdir ) custom_name="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            notRandom | notrandom ) random=0;;
            noBuild | nobuild) build_ns3=0;;
            maxSim | maxsim) num_cores="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 ));;
            help ) helpFunction;;
            * );;
         esac
         ;;
      m ) montecarlo="$OPTARG" ;;
      c ) custom_name="$OPTARG" ;;
      r ) random=0 ;;
      b ) build_ns3=0 ;;
      n ) num_cores="$OPTARG" ;;
      h ) helpFunction ;;
      ? ) helpFunction ;; # Print helpFunction in case parameter is non-existent
   esac
done

# Build ns3 before simulating
if [ "$build_ns3" == "1" ]
then 
   "${ns3home}/ns3" build

    if [ "$?" != "0" ]; then
      printf "${red}Error while building, simulation cancelled! ${clear}\n"
      exit 1
    fi

   clear
   echo "NS3 was built!"
fi

# Pre-processing of the parameters
nowdate=`date +%Y%m%d_%H%M%S`

for param in "${parameters[@]}"; do

    if [ "$montecarlo" -gt 1 ]; then

        if ([ "$custom_name" == "" ] || [[ $custom_name = *" "* ]])
        then
            batch_dir_name="PAR_BATCH-"${nowdate}
        else
            batch_dir_name="PAR-$custom_name"
        fi

        final_outdir="${batch_dir_name}/PAR-"$(bash -c "${basehome}/cca-perf.sh --outdir-print $param")
        outdir="${batch_dir_name}/PARAM${nparam}_NOTDONE"

        mkdir -p "$basehome/out/$outdir/outputs"
        echo "$param" > "$basehome/out/$outdir/parameters.txt"

        ((nparam++))
    fi

    for ((mont_num=1; mont_num<=montecarlo; mont_num++)); do

        # rem=$((sim_num % num_cores))

        param2=$param;
        if [ "$random" == "1" ]; then
            param2="$param -z $mont_num"
        fi

        stdoutTxt=$basehome/out/$outdir/outputs/sim${mont_num}.txt
        _cmd="${basehome}/cca-perf.sh -o $outdir/SIM${mont_num} $param2 &> $stdoutTxt"
        _cmd="${_cmd}; if [ "'$('"find ${basehome}/out/$outdir -type f -name .still_running | wc -l) -eq 0 ]; then mv ${basehome}/out/$outdir ${basehome}/out/$final_outdir; fi"
        _cmds_to_run+=("${_cmd}")
        
# Agregar al final de la línea 119 para que se haga el violin por cada PARAM, para luego juntarlos todos

        # (bash "cca-perf.sh" -o "$outdir/SIM${mont_num}" $param2 &> $stdoutTxt; printf "Done $sim_num ${TXT_RED}-${TXT_CLEAR} Exit Status $?\n") &
        # printf "[sim:${TXT_BLUE}${sim_num}${TXT_CLEAR} pid:${TXT_CYAN}$!${TXT_CLEAR}] Called ${TXT_GREEN}cca-perf.sh -o \"SIM${mont_num}\" ${param2} ${TXT_CLEAR}\n"

        # if [ "$rem" == "$((num_cores-1))" ]; then
        #     jobs
        #     wait
        # fi

        ((sim_num++))
    done
    
done

# Write parallel parameters
printf "\n${TXT_MAGENTA}Parallel parameters $TXT_CLEAR\n"
printf "Number of Iterations: ${montecarlo}\n" | tee -a $basehome/out/PAR-${custom_name}/parallel_parameters.txt
printf "Maximum number of simultaneous simulations: ${num_cores}\n" | tee -a $basehome/out/PAR-${custom_name}/parallel_parameters.txt
printf "Custom directory name: PAR-${custom_name}\n"
if [ "$random" -eq 0 ]; then
    printf "Different rng seeds: Deactivated\n" | tee -a $basehome/out/PAR-${custom_name}/parallel_parameters.txt
else
    printf "Different rng seeds: Activated\n" | tee -a $basehome/out/PAR-${custom_name}/parallel_parameters.txt
fi
if [ "$build_ns3" -eq 1 ]; then
    printf "NS3 was built before running simulations!\n" | tee -a $basehome/out/PAR-${custom_name}/parallel_parameters.txt
else
    printf "NS3 was not built before running simulations!\n" | tee -a $basehome/out/PAR-${custom_name}/parallel_parameters.txt
fi
echo

# Parallel processing
printf "${TXT_BLUE}Running simulations... $TXT_CLEAR\n"
printf "%s\n" "${_cmds_to_run[@]}" | xargs --max-procs=$num_cores -I % bash -c "%"

printf "Simulation finished!"
echo
printf "Running graph_parallel.py.."
echo
echo graph_parallel.py $RUTA_PROBE/out/$batch_dir_name
echo
python3 graph_parallel.py $RUTA_PROBE/out/$batch_dir_name