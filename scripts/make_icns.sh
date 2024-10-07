#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Please provide the name of your PNG file (without extension)"
    exit 1
fi

BASE_NAME=$1

if [ ! -f "${BASE_NAME}.png" ]; then
    echo "Error: ${BASE_NAME}.png does not exist"
    exit 1
fi

if ! command -v magick &> /dev/null
then
    echo "ImageMagick is not installed. Please install it to create .ico files."
    exit 1
fi

if [ $# -eq 0 ]; then
    echo "Please provide the name of your PNG file (without extension)"
    exit 1
fi

# Create iconset directory
mkdir "${BASE_NAME}.iconset"

# Generate scaled versions for ICNS
sips -z 16 16     "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_16x16.png"
sips -z 32 32     "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_16x16@2x.png"
sips -z 32 32     "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_32x32.png"
sips -z 64 64     "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_32x32@2x.png"
sips -z 128 128   "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_128x128.png"
sips -z 256 256   "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_128x128@2x.png"
sips -z 256 256   "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_256x256.png"
sips -z 512 512   "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_256x256@2x.png"
sips -z 512 512   "${BASE_NAME}.png" --out "${BASE_NAME}.iconset/icon_512x512.png"
cp "${BASE_NAME}.png" "${BASE_NAME}.iconset/icon_512x512@2x.png"

# Create icns file
iconutil -c icns "${BASE_NAME}.iconset"

# Create ico file
magick "${BASE_NAME}.png" -define icon:auto-resize=16,24,32,48,64,72,96,128,256 "${BASE_NAME}.ico"

# Remove temporary iconset directory
rm -R "${BASE_NAME}.iconset"

echo "ICNS file created: ${BASE_NAME}.icns"
echo "ICO file created: ${BASE_NAME}.ico"
