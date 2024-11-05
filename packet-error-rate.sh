#!/usr/bin/bash

# Load libs
source lib/cca-perf-lib.sh

tic=$(date +%s)

# Setting Folders
outfolder="./out"

myfolder=$outfolder

if [ $# -gt 0 ]; then myfolder=$1; fi

echo $myfolder

DEBUG=1

# serverID1=`grep serverID1 ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# serverID2=`grep serverID2 ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# serverID3=`grep serverID3 ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# serverIDs=( $serverID1 $serverID2 $serverID3)
# if [ $DEBUG = "1" ]; then 
#     echo "serverIDs: ${serverIDs[*]}"
# fi

# tcpTypeId1=`grep 'tcpTypeId1 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# tcpTypeId2=`grep 'tcpTypeId2 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# tcpTypeId3=`grep 'tcpTypeId3 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# tcpTypeIds=( $tcpTypeId1 $tcpTypeId2 $tcpTypeId3)
# if [ $DEBUG = "1" ]; then 
#     echo tcpTypeIds: "${tcpTypeIds[*]}"
# fi

# flowType=`grep 'flowType ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# flowType1=`grep 'flowType1 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# flowType2=`grep 'flowType2 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# flowType3=`grep 'flowType3 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# flowTypes=( $flowType1 $flowType2 $flowType3)
# if [ $DEBUG = "1" ]; then 
#     echo "flowTypes: ${flowTypes[*]}"
# fi

# UEN1=`grep 'UEN1 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# UEN2=`grep 'UEN2 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# UEN3=`grep 'UEN3 = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# UENum=`grep 'UENum = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`
# UENs=( $UEN1 $UEN2 $UEN3)
# if [ $DEBUG = "1" ]; then 
#     echo "UENs: ${UENs[*]}"
# fi
# ip_net_UE=`grep 'ip_net_UE = ' ${myfolder}/graph.ini | head -1| cut -d " " -f 3`


# echo "${serverIDs[*]}"
# echo "flowType: "$flowType
# for serverID in ${serverIDs[*]}
# do
#     printf "${serverID}." ;
# done

# filename="tcp-all-ascii.txt"
# if [ -f $myfolder/$filename.gz ]; then
# {
#     printf "Uncompressing $filename.gz ..." 
#     gunzip $myfolder/$filename.gz
# }
# fi

# printf "\nRemoving files ..." 
# for (( i=0; i<3; i++ )); do
#     if [ -f $myfolder/tcp-type-${i}-tmp.txt ]; then      rm $myfolder/tcp-type-${i}-tmp.txt;      fi
#     if [ -f $myfolder/tcp-type-${i}-tmp.txt.gz ]; then   rm $myfolder/tcp-type-${i}-tmp.txt.gz;   fi
#     if [ -f $myfolder/tcp-type-${i}-delay.txt ]; then    rm $myfolder/tcp-type-${i}-delay.txt;    fi
#     if [ -f $myfolder/tcp-type-${i}-delay.txt.gz ]; then rm $myfolder/tcp-type-${i}-delay.txt.gz; fi
#     if [ -f $myfolder/tcp-type-${i}-per.txt ]; then      rm $myfolder/tcp-type-${i}-per.txt;      fi
#     if [ -f $myfolder/tcp-type-${i}-per.txt.gz ]; then   rm $myfolder/tcp-type-${i}-per.txt.gz;   fi
    
# done

# printf "\nProcessing ..." 
# UEN=0
# for (( i=0; i<3; i++ )); do
#     printf "$(( $i + 1 ))." 
    
#     if [ $DEBUG = "1" ]; then 
#         echo $i, $UEN, ${UENs[$i]}, ${UEN} - $((${UEN}+${UENs[$i]})), ${tcpTypeIds[$i]}, ${flowTypes[$i]}, ${serverIDs[$i]}
#     fi

#     if [ ${flowTypes[$i]} = "TCP" ] ; then

#         filter="/NodeList/${serverIDs[$i]}/"
#         if [ $DEBUG = "1" ]; then 
#             echo "filter: ${filter}"
#         fi
#         grep -E "$filter" $myfolder/$filename | awk '\
#         {
#             if($1=="t"){
#                 server=$24;
#                 client=substr($26,1,length($26)-1);
#                 prot=$16;
#                 len=$23;
#                 size=substr($38,7,length($38)-7);
#                 if($31=="[SYN]"){size=1};
#                 if(NF<38){size=0};
#                 SN=substr($32,5,length($32)-4)+size;\
#                 T[client, server, SN]=$2;
#                 TF[client, server, SN]=0;
#                 L[client, server, SN]+=len;
#                 ACK[client, server, SN]=0;

#                 Btx[$2]+=len;
#                 Bdrop[$2]+=len;
#                 Ptx[$2]++;     
#                 Pdrop[$2]++;
#             }
#             if($1=="r"){
#                 client=$24;
#                 server=substr($26,1,length($26)-1);
#                 prot=$16;
#                 len=$23;
#                 size=substr($38,7,length($38)-7);
#                 if($31=="[SYN]"){size=1};
#                 if(NF<38){size=0};
#                 if ($36=="ns3::TcpOptionSack(blocks:"){
#                     blocks=$37
#                     split(blocks,tmp, ",");
#                     for (i=3; i <= tmp[1]*2; i++){
#                         split(tmp[i],serial, "]");
#                         SN=serial[1];
#                         if(ACK[client, server, SN]==0){
#                             TF[client, server, SN]=$2;
#                             Bdrop[T[client, server, SN]]-=L[client, server, SN];
#                             Pdrop[T[client, server, SN]]--;
#                             ACK[client, server, SN]=1;
#                         }
#                     }
#                 }else{
#                     SN=substr($33,5,length($33)-4);
#                     if(ACK[client, server, SN]==0){
#                         TF[client, server, SN]=$2;
#                         Bdrop[T[client, server, SN]]-=L[client, server, SN];
#                         Pdrop[T[client, server, SN]]--;
#                         ACK[client, server, SN]=1;
#                     }
#                 }

#             }

#         }
#         END {\
#             OFS="\t"; 
#                 print "RTT", "Time","UE","seq","rtt";
#                 for (c in T) {
#                 split(c,tmp, SUBSEP);
#                 client=tmp[1];
#                 server=tmp[2];
#                 split( client, tmpIP , ".");
#                 UE=tmpIP[4]-1;
#                 SN=tmp[3];
#                 if (TF[c] >0) print "RTT", T[c], UE, SN, (TF[c]-T[c]);
#             }
#             print "PER","Time","BytesTx","BytesDroped","PacketsTx","PacketsDroped";
#                 for (c in Btx) {print  "PER", c, Btx[c],Bdrop[c],Ptx[c],Pdrop[c]} 

#         } 
#         '  > $myfolder/tcp-type-${i}-tmp.txt

#     elif [ ${flowTypes[$i]} = "UDP" ] ; then

#         # prepare filter
#         filter=""
#         for (( j=$(( ${UEN} + 1 )); j < $(( ${UEN} + ${UENs[$i]} + 1 )); j++ )); do  
            
#             filter="${filter}/NodeList/$j/|"
#         done
#         # filter=${filter:0:-1}
#         filter="${filter}|/NodeList/${serverIDs[$i]}/"
        

#         grep -E "$filter" $myfolder/$filename | awk '\
#         {
#             if($1=="t"){
#                 server=$24;
#                 client=substr($26,1,length($26)-1);
#                 prot=$16;
#                 len=$23;
#                 SN=substr($34, 7, 6);
#                 T[client, server, SN]=$2;
#                 TF[client, server, SN]=0;
#                 L[client, server, SN]+=len;
#                 ACK[client, server, SN]=0;

#                 Btx[$2]+=$23;
#                 Ptx[$2]++;     
#                 Bdrop[$2]+=$23;
#                 Pdrop[$2]++;
#             }
#             if($1=="r"){
#                 server=$24;
#                 client=substr($26,1,length($26)-1);
#                 prot=$16;
#                 len=$23;
#                 SN=substr($34, 7, 6);
#                 if(ACK[client, server, SN]==0){
#                     TF[client, server, SN]=$2;
#                     ACK[client, server, SN]=1;

#                     Bdrop[T[client, server, SN]]-=L[client, server, SN];
#                     Pdrop[T[client, server, SN]]--;
#                 }
                    
#             }
#         }
#         END {\
#             OFS="\t"; 
#             print "RTT", "Time","UE","seq","rtt";
#             for (c in T) {
#                 split(c,tmp, SUBSEP);
#                 client=tmp[1];
#                 server=tmp[2];
#                 split( client, tmpIP , ".");
#                 UE=tmpIP[4]-1;
#                 SN=tmp[3];
#                 if (TF[c] >0) print "RTT", T[c], UE, SN, (TF[c]-T[c]);
#             }
#             print "PER","Time","BytesTx","BytesDroped","PacketsTx","PacketsDroped";
#                 for (c in Btx) {print  "PER", c, Btx[c],Bdrop[c],Ptx[c],Pdrop[c]} 

#         } 
#         '  > $myfolder/tcp-type-${i}-tmp.txt



#     fi
#     grep '^RTT' $myfolder/tcp-type-${i}-tmp.txt | awk '{if(NF==5){for(i=2;i<=NF;i++) printf $i"\t"; print ""}}' | sort -n > $myfolder/tcp-type-$((${i}+1))-delay.txt
#     grep '^PER' $myfolder/tcp-type-${i}-tmp.txt | awk '{if(NF==6){for(i=2;i<=NF;i++) printf $i"\t"; print ""}}' | sort -n > $myfolder/tcp-type-$((${i}+1))-per.txt

#     UEN=$(( ${UEN} + ${UENs[$i]} ));
# done

printf "\nProcessing ..."

python3 packet-error-rate.py $myfolder
# Concat
cat $myfolder/ip-group-?-delay.txt | sort -n |sed -e '1,2d' > $myfolder/tcp-all-delay.txt
cat $myfolder/ip-group-?-per.txt | awk '
    {
        c=$1;
        Btx[c]+=$2;
        Bdrop[c]+=$3;
        Ptx[c]+=$4;
        Pdrop[c]+=$5
    
    }
    END {\
        OFS="\t"; 
        print "Time","BytesTx","BytesDroped","PacketsTx","PacketsDroped";
        for (c in Btx) {print  c, Btx[c],Bdrop[c],Ptx[c],Pdrop[c]} 

    }
    ' | sort -n |sed -e '1d' > $myfolder/tcp-all-per.txt


# if [ -f $myfolder/$filename ]; then
# {
#     printf "Compressing..." 
#     gzip $myfolder/$filename 
# }
# fi
toc=$(date +%s)
printf "\nElapsed Time: "${TXT_MAGENTA}$(($toc-$tic))${TXT_CLEAR}" seconds\n"

exit
