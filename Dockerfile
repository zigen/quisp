#!/usr/bin/env docker build --build-arg VERSION=5.6 -t omnetpp/omnetpp-gui:u18.04-5.6 .
FROM omnetpp/omnetpp-base:u18.04 as base
RUN apt-get update -y && apt install -y gnupg2 && \
    echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-11 main" >> /etc/apt/sources.list && \
    echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-11 main" >> /etc/apt/sources.list && \
    wget -O tmp.key https://apt.llvm.org/llvm-snapshot.gpg.key && apt-key add tmp.key && \
    apt-get update -y && apt install -y --no-install-recommends \
    qt5-default libqt5opengl5-dev libgtk-3-0 libwebkitgtk-3.0-0 default-jre \
    osgearth libeigen3-dev cmake clang-11 clang-tidy-11 clang-format-11 && \
    ln -s -f /usr/bin/clang-11 /usr/bin/clang && \
    ln -s -f /usr/bin/clang++-11 /usr/bin/clang++ && \
    ln -s -f /usr/bin/clang-tidy-11 /usr/bin/clang-tidy && \
    ln -s -f /usr/bin/clang-format-11 /usr/bin/clang-format 

# first stage - build OMNeT++ with GUI
FROM base as builder

ARG VERSION
WORKDIR /root
RUN wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-$VERSION/omnetpp-$VERSION-src-linux.tgz \
         --referer=https://omnetpp.org/ -O omnetpp-src-linux.tgz --progress=dot:giga && \
         tar xf omnetpp-src-linux.tgz && rm omnetpp-src-linux.tgz
RUN mv omnetpp-$VERSION omnetpp
WORKDIR /root/omnetpp
ENV PATH /root/omnetpp/bin:$PATH
# remove unused files and build
RUN ./configure WITH_OSG=no　&& \
    make -j $(nproc) PREFER_CLANG=yes MODE=release base && \
    rm -r doc out test samples misc config.log config.status

# second stage - copy only the final binaries (to get rid of the 'out' folder and reduce the image size)
FROM base
ARG VERSION
RUN mkdir -p /root/omnetpp
WORKDIR /root/omnetpp
COPY --from=builder /root/omnetpp/ .
ENV PATH /root/omnetpp/bin:$PATH
RUN chmod 775 /root/ && \
    mkdir -p /root/quisp && \
    chmod 775 /root/quisp && \
    touch ide/error.log && chmod 666 ide/error.log && \
    mv bin/omnetpp bin/omnetpp.bak && \
    sed 's!$IDEDIR/../samples!/root/quisp!' bin/omnetpp.bak >bin/omnetpp && \
    rm bin/omnetpp.bak && chmod +x bin/omnetpp

# Google test need HACK
ARG GTEST_VERSION
RUN mkdir -p /root/clibrary
WORKDIR /root/clibrary
RUN chmod 755 /root/clibrary && \
    wget https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz -O gtest.tar.gz --progress=bar &&\
    tar -zxvf gtest.tar.gz &&\
    rm gtest.tar.gz &&\
    mv /root/clibrary/googletest-release-${GTEST_VERSION} /root/clibrary/googletest &&\
    chmod 755 /root/clibrary/googletest &&\
    cd /root/clibrary/googletest &&\
    mkdir build && chmod 755 build &&\
    cd /root/clibrary/googletest/build &&\
    cmake .. -DBUILD_SHARED_LIBS=0 &&\
    make
ENV GTEST_ROOT /root/clibrary/googletest/build/googletest/:$PATH
RUN echo 'PS1="quisp:\w\$ "' >> /root/.bashrc && chmod +x /root/.bashrc && \
    touch /root/.hushlogin
ENV HOME=/root/
CMD /bin/bash --init-file /root/.bashrc
WORKDIR /root/quisp
