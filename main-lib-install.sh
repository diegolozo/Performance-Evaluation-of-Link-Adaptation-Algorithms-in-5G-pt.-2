NOW=`date +%Y%m%d%H%M`

# Building model updates
cp ../../src/buildings/model/building.cc ../../src/buildings/model/building.cc.${NOW}
cp ../../src/buildings/model/building.h ../../src/buildings/model/building.h.${NOW}
cp ../../src/buildings/model/buildings-propagation-loss-model.h ../../src/buildings/model/buildings-propagation-loss-model.h.${NOW}
cp ../../src/buildings/model/buildings-propagation-loss-model.cc ../../src/buildings/model/buildings-propagation-loss-model.cc.${NOW}
cp src/buildings/model/* ../../src/buildings/model 

# MAC lena issue 159
cp ../../contrib/nr/model/nr-mac-scheduler-lcg.cc ../../contrib/nr/model/nr-mac-scheduler-lcg.cc.${NOW} 
cp contrib/nr/model/nr-mac-scheduler-lcg.cc ../../contrib/nr/model/. 

# NR AMC
cp ../../contrib/nr/model/nr-amc.cc ../../contrib/nr/model/nr-amc.cc.${NOW} 
cp ../../contrib/nr/model/nr-amc.h ../../contrib/nr/model/nr-amc.h.${NOW} 
cp contrib/nr/model/nr-amc.cc ../../contrib/nr/model/. 
cp contrib/nr/model/nr-amc.h ../../contrib/nr/model/. 

#  RxPacketTrace.txt lena issue 185
cp ../../contrib/nr/helper/nr-phy-rx-trace.cc ../../contrib/nr/helper/nr-phy-rx-trace.cc.${NOW} 
cp contrib/nr/helper/nr-phy-rx-trace.cc ../../contrib/nr/helper/. 

# RLC buffer stats
cp ../../src/lte/model/lte-rlc-um.cc  ../../src/lte/model/lte-rlc-um.cc.${NOW}
cp ../../src/lte/model/lte-rlc-um.h  ../../src/lte/model/lte-rlc-um.h.${NOW}
cp src/lte/model/lte-rlc-um.cc  ../../src/lte/model/.
cp src/lte/model/lte-rlc-um.h  ../../src/lte/model/.

# RLC for adding flags to buffers
cp ../../src/lte/model/lte-enb-rrc.cc  ../../src/lte/model/lte-enb-rrc.cc.ori
cp ../../src/lte/model/lte-ue-rrc.h  ../../src/lte/model/lte-ue-rrc.h.ori
cp src/lte/model/lte-enb-rrc.cc  ../../src/lte/model/.
cp src/lte/model/lte-ue-rrc.cc  ../../src/lte/model/.

# Better QUIC message for measuring rtt
cp ../../src/quic/model/quic-subheader.cc ../../src/quic/model/quic-subheader.cc.${NOW}
cp ../../src/quic/model/quic-header.cc ../../src/quic/model/quic-header.cc.${NOW}
cp ../../src/quic/model/quic-transport-parameters.cc ../../src/quic/model/quic-transport-parameters.cc.${NOW}
cp ../../src/quic/model/quic-socket-tx-scheduler.cc ../../src/quic/model/quic-socket-tx-scheduler.cc.${NOW}

cp src/quic/model/quic-subheader.cc ../../src/quic/model/.
cp src/quic/model/quic-header.cc ../../src/quic/model/.
cp src/quic/model/quic-transport-parameters.cc ../../src/quic/model/.
cp src/quic/model/quic-socket-tx-scheduler.cc ../../src/quic/model/.

# BBR ns-3 issues 
cp ../../src/internet/model/tcp-bbr.cc ../../src/internet/model/tcp-bbr.cc.${NOW}
cp ../../src/internet/model/tcp-bbr.h ../../src/internet/model/tcp-bbr.h.${NOW}
cp src/internet/model/tcp-bbr.cc ../../src/internet/model/.
cp src/internet/model/tcp-bbr.h ../../src/internet/model/.

# TCP buffer stats
cp ../../src/internet/model/tcp-rx-buffer.cc ../../src/internet/model/tcp-rx-buffer.cc.${NOW}
cp src/internet/model/tcp-rx-buffer.cc ../../src/internet/model/.

# ns3 issue 966
cp ../../src/internet/model/tcp-cubic.h             src/internet/model/.
cp ../../src/internet/model/tcp-cubic.cc            src/internet/model/.
cp ../../src/internet/model/tcp-dctcp.cc            src/internet/model/.
cp ../../src/internet/model/tcp-linux-reno.cc       src/internet/model/.
cp ../../src/internet/model/tcp-linux-reno.h        src/internet/model/.
cp ../../src/internet/model/tcp-socket-base.cc      src/internet/model/.
cp ../../src/internet/model/tcp-socket-state.h      src/internet/model/.
cp ../../src/test/ns3tcp/ns3tcp-cubic-test-suite.cc src/test/ns3tcp/.

# 3GPP Propagation Loss Model Custom

cp ../../src/propagation/model/three-gpp-propagation-loss-model.h ../../src/propagation/model/three-gpp-propagation-loss-model.h.${NOW}
cp ../../src/propagation/model/three-gpp-propagation-loss-model.cc ../../src/propagation/model/three-gpp-propagation-loss-model.cc.${NOW}
cp src/propagation/model/three-gpp-propagation-loss-model.h ../../src/propagation/model/three-gpp-propagation-loss-model.h
cp src/propagation/model/three-gpp-propagation-loss-model.cc ../../src/propagation/model/three-gpp-propagation-loss-model.cc

# # better debug
# cp ../../contrib/nr/model/nr-ue-mac.cc  ../../contrib/nr/model/nr-ue-mac.cc.${NOW}
# cp ../../contrib/nr/model/nr-ue-phy.cc  ../../contrib/nr/model/nr-ue-phy.cc.${NOW}
# cp ../../contrib/nr/model/nr-gnb-mac.cc ../../contrib/nr/model/nr-gnb-mac.cc.${NOW}
# cp ../../contrib/nr/model/nr-gnb-phy.cc ../../contrib/nr/model/nr-gnb-phy.cc.${NOW}
# cp contrib/nr/model/nr-ue-mac.cc ../../contrib/nr/model/.
# cp contrib/nr/model/nr-ue-phy.cc ../../contrib/nr/model/.
# cp contrib/nr/model/nr-gnb-mac.cc ../../contrib/nr/model/.
# cp contrib/nr/model/nr-gnb-phy.cc ../../contrib/nr/model/.


# # other fix
# cp ../../contrib/nr/model/nr-mac-scheduler-ns3.cc ../../contrib/nr/model/nr-mac-scheduler-ns3.cc.${NOW}
# cp contrib/nr/model/nr-mac-scheduler-ns3.cc ../../contrib/nr/model/.

