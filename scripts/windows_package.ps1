New-Item -ItemType Directory -Path ".\build"
Set-Location build
$QT_ROOT="C:\Qt\5.14.2\msvc2017_64"
$CMAKE="C:\Qt\Tools\CMake_64\bin\cmake.exe"
$DEPLOY="$QT_ROOT\bin\windeployqt.exe"
& $CMAKE .. -DCMAKE_PREFIX_PATH="$QT_ROOT\lib\cmake" `
  -DEIGEN3_INCLUDE_DIR="include" `
  -DEIGEN_DONT_ALIGN_STATICALLY=ON `
  -DCMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD=ON `
  -DCMAKE_INSTALL_PREFIX=out

& $CMAKE --build . --config Release
& $CMAKE --build . --config Release --target install
Write-Output "Deploying Qt"
& $DEPLOY --verbose 0 --no-compiler-runtime --no-angle --no-opengl-sw out\CrystalExplorer.exe
Write-Output "Bundling installer"
$ISCC="C:\Program Files (x86)\Inno Setup 6\ISCC.exe" 
& $ISCC /Q /DPREFIX=".\out" setup.iss
Set-Location ..
Write-Output "Complete"