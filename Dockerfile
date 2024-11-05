# Docker file to install ns3, ns3-ai, 5GLENA and cca-perf, all in one.
# Ubuntu 22.04 (jammy)

FROM ubuntu:22.04
LABEL maintainer="AGDT"

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get clean && apt-get install -y build-essential
RUN apt-get install software-properties-common -y


# User 
RUN apt-get install -y sudo
RUN useradd --create-home --shell /bin/bash -G sudo jsandova
RUN echo "jsandova:jsandova" | chpasswd

RUN apt-get update
RUN apt-get install -y git cmake libboost-all-dev vim g++ python3.10

# Move to home dir
WORKDIR /home/jsandova 

COPY docs/ns3_prerequisites.sh /ns3_prerequisites.sh
RUN chmod +x /ns3_prerequisites.sh

RUN /ns3_prerequisites.sh
RUN mv ~/* /home/jsandova
RUN chown -R  jsandova: /home/jsandova/
RUN chmod -R u+rwx /home/jsandova

USER jsandova
RUN echo 'PATH="~/ns-3-dev/:$PATH"' >> /home/jsandova/.bashrc

RUN echo "echo [!] To Install QUIC module for ns-3.41, see the tutorial in this repository, you have to change some files manually." >> /home/jsandova/.bashrc
RUN echo "echo [!] https://github.com/signetlabdei/quic/tree/release-3-41" >> /home/jsandova/.bashrc
RUN echo "echo [!] Remember to checkout to release-3-41 branch after cloning the QUIC repository" >> /home/jsandova/.bashrc
RUN echo "echo If already installed, delete this lines from ~/.bashrc" >> /home/jsandova/.bashrc

ENTRYPOINT ["tail"]
CMD ["-f","/dev/null"]