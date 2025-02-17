#!/usr/bin/env bash
BUILD_DIR="build"
ARCH="x86_64"
NAME="linux-x86_64-static"

# Download required tools
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

cmake . -B"${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release \
    -GNinja \
    -DCMAKE_CXX_FLAGS="-O2 -static-libgcc -static-libstdc++" \
    -DCMAKE_C_FLAGS="-O2 -static-libgcc" \
    -DCPACK_SYSTEM_NAME="${NAME}" -DCMAKE_INSTALL_PREFIX="/usr"

cmake --build "${BUILD_DIR}"
DESTDIR=AppDir cmake --install "${BUILD_DIR}" --prefix /usr

# Create AppImage
export QMAKE="$(which qmake)"
./linuxdeploy-x86_64.AppImage --appdir=AppDir \
    --plugin qt \
    --output appimage \
    --icon-file=icons/CrystalExplorer512x512.png \
    --desktop-file=resources/crystalexplorer.desktop
