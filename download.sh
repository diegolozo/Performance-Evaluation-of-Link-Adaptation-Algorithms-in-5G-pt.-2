#!/usr/bin/bash
folder_src="~/ns-3.41/scratch/cca-perf/out/"
folder_dst="/mnt/g/Mi unidad/doctorado/2024/5G-LENA/ns-3.41/scratch/cca-perf/out/"
ccas=("UDP" "TcpBbr" "TcpHighSpeed" "TcpCubic" "TcpNewReno")
files=("-UDP-UDP-E-010-UE_2_0_0-AQM_None-0_60-600" "-UDP-UDP-E-100-UE_2_0_0-AQM_None-0_60-600" "-UDP-UDP-E-200-UE_2_0_0-AQM_None-0_60-600" "-UDP-UDP-E-300-UE_2_0_0-AQM_None-0_60-600" "-UDP-UDP-E-400-UE_2_0_0-AQM_None-0_60-600" "-UDP-UDP-E-500-UE_2_0_0-AQM_None-0_60-600")
it=22

for cca in "${ccas[@]}"; do
    for f in "${files[@]}"; do
        file="0-${cca}${f}"
        tag=0
        for i in $( eval echo {$(($tag + 0))..$(($tag + $it -1))} ); do
            tag=$(printf "%02d" $i)
            echo "${file}-${tag}"
            mkdir -p "${folder_dst}${file}-${tag}"
            scp -q jsandova@dificil:${folder_src}${file}-${tag}/*.\{png,ini\} "${folder_dst}${file}-${tag}/"
        done
    done
done

ccas=("TcpBbr" "TcpNewReno")
aqms=("RED90" "RED80" "RED70")

for cca in "${ccas[@]}"; do
    for aqm in "${aqms[@]}"; do
        file="0-${cca}-UDP-UDP-E-200-UE_1_1_0-AQM_${aqm}-0_60-600"
        tag=0
        for i in $( eval echo {$(($tag + 0))..$(($tag + $it -1))} ); do
            tag=$(printf "%02d" $i)
            echo "${file}-${tag}"
            mkdir -p "${folder_dst}${file}-${tag}"
            scp -q jsandova@dificil:${folder_src}${file}-${tag}/*.\{png,ini\} "${folder_dst}${file}-${tag}/"
        done
    done
done
         

