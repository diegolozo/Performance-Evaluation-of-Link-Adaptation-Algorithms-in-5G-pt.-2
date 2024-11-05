#/usr/bin/bash

# MAC lena issue 159
mkdir -p contrib/nr/model/
cp ../../contrib/nr/model/nr-mac-scheduler-lcg.cc contrib/nr/model/.

# MAC lena issue 185
mkdir -p contrib/nr/helper/
cp ../../contrib/nr/helper/nr-phy-rx-trace.cc contrib/nr/helper/.

# RLC buffer stats
mkdir -p src/lte/model/
cp ../../src/lte/model/lte-rlc-um.cc src/lte/model/.
cp ../../src/lte/model/lte-rlc-um.h src/lte/model/.

# Better QUIC message for measuring rtt
mkdir -p src/quic/model/
cp ../../src/quic/model/quic-subheader.cc src/quic/model/.
cp ../../src/quic/model/quic-header.cc src/quic/model/.
cp ../../src/quic/model/quic-transport-parameters.cc src/quic/model/.
cp ../../src/quic/model/quic-socket-tx-scheduler.cc src/quic/model/.

# BBR ns-3 issues
mkdir -p src/internet/model/
cp ../../src/internet/model/tcp-bbr.cc src/internet/model/.
cp ../../src/internet/model/tcp-bbr.h src/internet/model/.

# TCP buffer stats
cp ../../src/internet/model/tcp-rx-buffer.cc src/internet/model/.


# better debug
# mkdir -p contrib/nr/model/
# cp ../../contrib/nr/model/nr-ue-mac.cc contrib/nr/model/.
# cp ../../contrib/nr/model/nr-ue-phy.cc contrib/nr/model/.
# cp ../../contrib/nr/model/nr-gnb-mac.cc contrib/nr/model/.
# cp ../../contrib/nr/model/nr-gnb-phy.cc contrib/nr/model/.


# # other fix
# mkdir -p contrib/nr/model/
# cp ../../contrib/nr/model/nr-mac-scheduler-ns3.cc contrib/nr/model/.
