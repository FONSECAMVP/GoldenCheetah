#!/bin/bash
set -ev

sudo apt-get update -qq
sudo apt-get install -qq flex libpulse-dev
sudo apt-get install -qq libglu1-mesa-dev libxcb-cursor-dev
sudo apt-get install -qq libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install -qq libsamplerate0-dev
sudo apt-get install -qq libical-dev

# R 4.0
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu jammy-cran40/"
sudo apt-get update -qq
sudo apt-get install -qq r-base-dev
R --version

# D2XX - marker-gated: D2XX/.gc-d2xx-complete (written last, after the whole
# release/ tree is extracted) means D2XX/ is populated; anything else
# refetches, and a 403/short archive prints one WARNING instead of aborting
# the leg under set -e.
D2XX_VERSION=1.4.27
if [ ! -f D2XX/.gc-d2xx-complete ] || [ "$(cat D2XX/.gc-d2xx-complete)" != "$D2XX_VERSION" ]; then
    rm -rf D2XX D2XX.tmp
    mkdir -p D2XX.tmp
    if wget --no-verbose -O D2XX.tmp/libftd2xx.tgz "${D2XX_URL_LINUX:-https://ftdichip.com/wp-content/uploads/2022/07/libftd2xx-x86_64-1.4.27.tgz}" \
        && tar xf D2XX.tmp/libftd2xx.tgz -C D2XX.tmp \
        && [ -d D2XX.tmp/release ] \
        && rm -f D2XX.tmp/libftd2xx.tgz \
        && echo "$D2XX_VERSION" > D2XX.tmp/.gc-d2xx-complete \
        && mv D2XX.tmp D2XX; then
        :
    else
        echo "WARNING: D2XX archive fetch/extract failed; building without D2XX support."
        rm -rf D2XX.tmp
        mkdir -p D2XX
    fi
fi

# SRMIO - gated on srmio's own build artifact (.libs/libsrmio.a), not a
# non-empty directory (a failed genautomake.sh/configure/make
# left srmio/ non-empty but unbuilt, and SAVE_CACHE_ON_ERROR cached that).
if [ ! -f srmio/.libs/libsrmio.a ]; then
    rm -rf srmio
    sudo apt-get install -qq autoconf automake libtool build-essential
    git clone https://github.com/rclasen/srmio.git
    cd srmio
    sh genautomake.sh
    ./configure --disable-shared --enable-static
    make --silent -j2
    cd ..
fi
cd srmio
sudo make install
cd ..

# LIBUSB
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev

# GSL
sudo apt-get -qq install libgsl-dev

# Python ${PYTHON_VERSION} for embedding
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get update -qq
sudo apt-get install -qq python${PYTHON_VERSION} python${PYTHON_VERSION}-dev python${PYTHON_VERSION}-venv
python${PYTHON_VERSION} --version

# Install fuse2 required to run older AppImages, and patchelf to fix QtWebEngineProcess
sudo add-apt-repository -y universe
sudo apt install libfuse2 patchelf

exit
