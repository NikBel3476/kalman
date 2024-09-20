FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get -y install \
    build-essential \
    python3 \
    python3-pip \
    git \
    wget \
    curl \
    ninja-build

# WORKDIR /usr/local/gcc14_build
# RUN wget http://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.gz
# RUN tar -xf gcc-14.1.0.tar.gz

# WORKDIR /usr/local/gcc14_build/gcc-14.1.0
# RUN ./configure -v --build=x86_64-linux-gnu \
    # --host=x86_64-linux-gnu \
    # --target=x86_64-linux-gnu \
    # --prefix=/usr/local/gcc-14.1.0 \
    # --enable-checking=release \
    # --enable-languages=c,c++ \
    # --disable-multilib \
    # --program-suffix=-14.1.0
# RUN make -j$(nproc --all)
# RUN make install
# ENV PATH="/usr/local/gcc-14.1.0/bin:$PATH"

WORKDIR /usr/local/cmake_build
RUN wget https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-linux-x86_64.tar.gz
RUN mkdir -p /usr/local/cmake
RUN tar -xf cmake-3.30.3-linux-x86_64.tar.gz
RUN mv cmake-3.30.3-linux-x86_64/* /usr/local/cmake
ENV PATH="/usr/local/cmake/bin:$PATH"
RUN cmake --version

# WORKDIR /usr/local/pyenv
# RUN git clone --branch v2.4.13 https://github.com/pyenv/pyenv.git .
# ENV PYENV_ROOT="/usr/local/pyenv"
# ENV PATH="$PYENV_ROOT/bin:$PATH"
# RUN eval "$(pyenv init -)"
# RUN pyenv install 3.12.5

WORKDIR /usr/local/clang_build
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/clang+llvm-18.1.8-x86_64-linux-gnu-ubuntu-18.04.tar.xz -O clang_18.1.8.tar.xz
RUN tar -xf clang_18.1.8.tar.xz
RUN mkdir -p /usr/local/clang
RUN mv clang+llvm-18.1.8-x86_64-linux-gnu-ubuntu-18.04/* /usr/local/clang
ENV PATH="/usr/local/clang/bin:$PATH"
RUN clang --version
RUN clang++ --version

WORKDIR /usr/local/qt6.8_build
RUN git clone --branch 6.8 git://code.qt.io/qt/qt5.git .
# RUN pyenv local 3.12.5
# RUN python -m venv venv
# RUN source venv/bin/activate
RUN ./init-repository
ENV CC="clang"
ENV CXX="clang++"
RUN apt-get -y install freeglut3-dev
RUN apt-get -y install zlib1g-dev
RUN apt-get -y install libtinfo5
RUN apt-get -y install terminator
RUN ./configure -release -prefix /usr/local/qt6.8 -submodules qtbase,qtserialport,qttranslations
RUN cmake --build . --parallel
RUN cmake --install .
ENV PATH="/usr/local/qt6.8/bin:$PATH"

