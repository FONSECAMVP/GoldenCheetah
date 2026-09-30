#!/bin/bash
set -ev

date
# Don't update or cleanup
export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

brew install bison@2.7
brew install gsl
brew install libical
brew upgrade libusb
brew install libsamplerate
rm -rf '/usr/local/include/c++'
sudo chmod -R +w /usr/local

# R 4.1.1
curl -L -O https://cran.r-project.org/bin/macosx/base/R-4.1.1.pkg
sudo installer -pkg R-4.1.1.pkg -target /
R --version

# SRMIO - gated on srmio's own build artifact (.libs/libsrmio.a), not a
# non-empty directory (B-STAGE9-178: a failed genautomake.sh/configure/make
# left srmio/ non-empty but unbuilt, and SAVE_CACHE_ON_ERROR cached that).
if [ ! -f srmio/.libs/libsrmio.a ]; then
    rm -rf srmio
    git clone https://github.com/rclasen/srmio.git
    cd srmio
    sh genautomake.sh
    ./configure --disable-shared --enable-static
    make -j2 --silent
    cd ..
fi
cd srmio
sudo make install
cd ..

# D2XX - marker-gated: D2XX/.gc-d2xx-complete (written last, after dylib +
# .a + headers are all copied) means D2XX/ is populated; anything else
# refetches, and a 403/short zip/incomplete dmg prints one WARNING instead of
# aborting the leg under set -e (B-STAGE9-172).
D2XX_VERSION=1.4.24
if [ ! -f D2XX/.gc-d2xx-complete ] || [ "$(cat D2XX/.gc-d2xx-complete)" != "$D2XX_VERSION" ]; then
    rm -rf D2XX D2XX.tmp
    mkdir -p D2XX.tmp
    if curl -sS -o D2XX.tmp/D2XX1.4.24.zip "${D2XX_URL:-https://ftdichip.com/wp-content/uploads/2021/05/D2XX1.4.24.zip}" \
        && unzip -tq D2XX.tmp/D2XX1.4.24.zip >/dev/null 2>&1 \
        && unzip -q D2XX.tmp/D2XX1.4.24.zip -d D2XX.tmp \
        && hdiutil attach D2XX.tmp/D2XX1.4.24.dmg -mountpoint D2XX.tmp/mnt -nobrowse -quiet \
        && cp D2XX.tmp/mnt/release/build/libftd2xx.1.4.24.dylib D2XX.tmp/ \
        && cp D2XX.tmp/mnt/release/build/libftd2xx.a D2XX.tmp/ \
        && cp D2XX.tmp/mnt/release/*.h D2XX.tmp/ \
        && hdiutil detach D2XX.tmp/mnt -quiet \
        && rm -rf D2XX.tmp/mnt D2XX.tmp/D2XX1.4.24.zip D2XX.tmp/D2XX1.4.24.dmg \
        && echo "$D2XX_VERSION" > D2XX.tmp/.gc-d2xx-complete \
        && mv D2XX.tmp D2XX; then
        :
    else
        hdiutil detach D2XX.tmp/mnt -quiet 2>/dev/null || true
        echo "WARNING (B-STAGE9-172): D2XX archive fetch/extract failed; building without D2XX support."
        rm -rf D2XX.tmp
        mkdir -p D2XX
    fi
fi
if [ -f D2XX/.gc-d2xx-complete ]; then
    sudo cp D2XX/libftd2xx.1.4.24.dylib /usr/local/lib
fi

# Python ${PYTHON_VERSION} for embedding (system Python is too old for sip-tools)
brew install python@${PYTHON_VERSION}
export PATH="/usr/local/opt/python@${PYTHON_VERSION}/bin:$PATH"
python3 --version
# Upgrade pip to ensure you have the latest version
python3 -m pip install --upgrade pip

exit
