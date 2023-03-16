FROM debian:bullseye-slim AS intermediate

# Install dependencies
RUN apt-get -qq update; \
    apt-get install -qqy --no-install-recommends \
        gnupg2 wget ca-certificates apt-transport-https \
        autoconf automake git cmake cmake-extras dpkg-dev file make patch libc6-dev

# Set repository key
RUN wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# Install
RUN echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-16 main" \
        > /etc/apt/sources.list.d/llvm.list; \
    apt-get -qq update && \
    apt-get install -qqy -t \
        llvm-toolchain-bullseye-16 clang-16 clang-tidy-16 clang-format-16 lld-16 libc++1-16 libc++-16-dev lldb-16 \
        nano \
        libvulkan-dev glslang-tools xorg-dev \
        libasound2-dev libjack-dev && \
    for f in /usr/lib/llvm-16/bin/*; do ln -sf "$f" /usr/bin; done && \
    ln -s /usr/lib/x86_64-linux-gnu/libc++abi.so.1 /usr/lib/x86_64-linux-gnu/libc++abi.so && \
    ln -sf /usr/bin/lld-16 /usr/bin/ld && \
    rm -rf /var/lib/apt/lists/*

FROM intermediate as final
