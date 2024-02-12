#!/bin/sh
BUILD_DIR=build-package
APP=CrystalExplorer.app
echo "Cleaning up old build"
#rm -rf $BUILD_DIR /tmp/$APP
rm -rf /tmp/$APP
mkdir $BUILD_DIR
meson $BUILD_DIR --buildtype=release  --prefix=/tmp/$APP --bindir=Contents/MacOS
ninja -C $BUILD_DIR install
echo "$APP built and packaged!"
rm -rf ./$APP
mv /tmp/$APP ./
