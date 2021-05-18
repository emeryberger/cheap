FROM gcc:10

RUN \
    apt-get -qq update && apt-get -qq install -y lsb-release wget software-properties-common gnupg2 git

RUN wget -nv -O - 'http://downloads.sourceforge.net/project/boost/boost/1.76.0/boost_1_76_0.tar.gz' \
        | tar xz \
    && ( \
        cd boost_1_76_0 \
        && ./bootstrap.sh --prefix=/usr/local \
        && ./b2 --with-json release install \
    ) \
    && rm -rf boost_1_76_0
