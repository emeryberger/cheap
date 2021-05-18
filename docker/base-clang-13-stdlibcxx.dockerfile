FROM gcc:10

RUN \
    apt-get -qq update && apt-get -qq install -y lsb-release wget software-properties-common gnupg2 git make \
    \
    && wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && wget -nv -O - https://apt.llvm.org/llvm.sh | bash -s -- 13 \
    && apt-get -qq install -y clang-format-13 clang-tidy-13

RUN wget -nv -O - https://gist.githubusercontent.com/junkdog/70231d6953592cd6f27def59fe19e50d/raw/update-alternatives-clang.sh | bash -s -- 13 100 \
    && update-alternatives --install /usr/bin/ld ld /usr/bin/ld.lld-13 100 \
    && rm -f /usr/bin/clang++ \
    && printf '#!/bin/sh\nexec "/usr/bin/clang++-13" --gcc-toolchain=/usr/local ${@}' >/usr/bin/clang++ \
    && chmod +x /usr/bin/clang++

RUN wget -nv -O - 'http://downloads.sourceforge.net/project/boost/boost/1.76.0/boost_1_76_0.tar.gz' \
        | tar xz \
    && ( \
        cd boost_1_76_0 \
        && ./bootstrap.sh --prefix=/usr/local \
        && ./b2 toolset=clang --with-json release install \
    ) \
    && rm -rf boost_1_76_0
