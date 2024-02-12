#!/bin/sh
QTF_PATH=/usr/local/Cellar/qt5/5.15.2/lib
APP_FRAMEWORKS=${MESON_INSTALL_PREFIX}/Contents/Frameworks
BINARY=${MESON_INSTALL_PREFIX}/Contents/MacOS/CrystalExplorer

function bundle_qt5 {
    echo "Installing $1 framework"
    cp -R $QTF_PATH/$1.framework $APP_FRAMEWORKS
    old_rpath=$(otool -L $BINARY | grep $1 | awk '{print $1;}')
    install_name_tool -change $old_rpath @executable_path/../Frameworks/$1.framework/Versions/5/$1 $BINARY
}

#mkdir -p ${MESON_INSTALL_PREFIX}/Contents/Frameworks
#bundle_qt5 QtCore
#bundle_qt5 QtGui
#bundle_qt5 QtWidgets
#bundle_qt5 QtOpenGL
macdeployqt ${MESON_INSTALL_PREFIX}
