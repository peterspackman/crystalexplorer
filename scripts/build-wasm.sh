#!/bin/bash

# Build script for WebAssembly (WASM) build
# Usage:
#   ./scripts/build-wasm.sh                    # Build WASM version
#   ./scripts/build-wasm.sh --clean            # Clean and rebuild
#   ./scripts/build-wasm.sh --setup-only       # Only setup dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-wasm"
DEPS_DIR="$PROJECT_ROOT/deps-wasm"

# Qt version to install
QT_VERSION="${QT_VERSION:-6.9.3}"
# Emscripten version (should match Qt's recommended version)
EMSDK_VERSION="${EMSDK_VERSION:-3.1.70}"

# Parse arguments
CLEAN_BUILD=false
SETUP_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --setup-only)
            SETUP_ONLY=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--clean] [--setup-only]"
            echo "  --clean       Clean build directory before building"
            echo "  --setup-only  Only setup dependencies (Emscripten and Qt)"
            echo ""
            echo "Environment variables:"
            echo "  QT_VERSION    Qt version to install (default: 6.8.1)"
            echo "  EMSDK_VERSION Emscripten version (default: 3.1.50)"
            exit 0
            ;;
        *)
            echo "Unknown option $1"
            exit 1
            ;;
    esac
done

echo "=== CrystalExplorer WASM Build Script ==="
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo "Dependencies directory: $DEPS_DIR"
echo "Qt version: $QT_VERSION"
echo "Emscripten version: $EMSDK_VERSION"

# Create dependencies directory
mkdir -p "$DEPS_DIR"

# Setup Emscripten SDK
EMSDK_DIR="$DEPS_DIR/emsdk"
if [[ ! -d "$EMSDK_DIR" ]]; then
    echo "Installing Emscripten SDK..."
    cd "$DEPS_DIR"
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install "$EMSDK_VERSION"
    ./emsdk activate "$EMSDK_VERSION"
else
    echo "Emscripten SDK already installed"
    cd "$EMSDK_DIR"
    # Ensure correct version is activated
    ./emsdk activate "$EMSDK_VERSION"
fi

# Source Emscripten environment
source "$EMSDK_DIR/emsdk_env.sh"

echo "Emscripten version: $(emcc --version | head -1)"

# Setup Qt for WASM using aqtinstall via uv
QT_DIR="$DEPS_DIR/Qt/$QT_VERSION"

# Check if uv is available
if ! command -v uv &> /dev/null; then
    echo "Error: uv not found. Please install uv first:"
    echo "  curl -LsSf https://astral.sh/uv/install.sh | sh"
    exit 1
fi

# Install desktop Qt (required for WASM build)
if [[ ! -d "$QT_DIR/macos" ]]; then
    echo "Installing Qt $QT_VERSION desktop (required for WASM)..."
    cd "$DEPS_DIR"
    uv run --with aqtinstall aqt install-qt mac desktop "$QT_VERSION" clang_64 -O Qt
else
    echo "Qt $QT_VERSION desktop already installed"
fi

# Install Qt WASM
if [[ ! -d "$QT_DIR/wasm_singlethread" ]] && [[ ! -d "$QT_DIR/wasm_multithread" ]]; then
    echo "Installing Qt $QT_VERSION for WASM..."

    cd "$DEPS_DIR"

    # Install Qt for WASM (single-threaded version) using uv
    # Note: Qt 6.7+ has both single and multi-threaded WASM builds
    echo "Installing Qt WASM single-threaded..."
    uv run --with aqtinstall aqt install-qt all_os wasm "$QT_VERSION" wasm_singlethread -O Qt

    # Optionally install multi-threaded version (uncomment if needed)
    # echo "Installing Qt WASM multi-threaded..."
    # uv run --with aqtinstall aqt install-qt all_os wasm "$QT_VERSION" wasm_multithread -O Qt
else
    echo "Qt $QT_VERSION for WASM already installed"
fi

# Determine which Qt WASM build to use
if [[ -d "$QT_DIR/wasm_singlethread" ]]; then
    QT_WASM_DIR="$QT_DIR/wasm_singlethread"
    WASM_VARIANT="singlethread"
elif [[ -d "$QT_DIR/wasm_multithread" ]]; then
    QT_WASM_DIR="$QT_DIR/wasm_multithread"
    WASM_VARIANT="multithread"
else
    echo "Error: Qt WASM installation not found"
    exit 1
fi

echo "Using Qt WASM: $QT_WASM_DIR ($WASM_VARIANT)"

# Exit if setup-only
if [[ "$SETUP_ONLY" == "true" ]]; then
    echo "✅ Setup completed successfully!"
    echo ""
    echo "To build, run: $0"
    echo "To use Emscripten manually, source: source $EMSDK_DIR/emsdk_env.sh"
    echo "Qt WASM path: $QT_WASM_DIR"
    exit 0
fi

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == "true" ]] && [[ -d "$BUILD_DIR" ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Configure CMake
echo "Configuring CMake for WASM..."
cd "$PROJECT_ROOT"

# Use Qt's WASM toolchain file (which properly sets up Emscripten + Qt paths)
QT_TOOLCHAIN="$QT_WASM_DIR/lib/cmake/Qt6/qt.toolchain.cmake"
QT_HOST_PATH="$QT_DIR/macos"

# Configure with Qt toolchain
cmake --preset wasm \
    -DCMAKE_TOOLCHAIN_FILE="$QT_TOOLCHAIN" \
    -DQT_HOST_PATH="$QT_HOST_PATH"

# Build
echo "Building..."
cmake --build --preset wasm

echo "✅ WASM build completed successfully!"
echo "Build output: $BUILD_DIR"
echo ""
echo "To run the application, you need to serve it with a web server that supports:"
echo "  - Cross-Origin-Opener-Policy: same-origin"
echo "  - Cross-Origin-Embedder-Policy: require-corp"
echo ""
echo "Example using Python:"
echo "  cd $BUILD_DIR"
echo "  python3 -m http.server 8000"
echo ""
echo "Then open: http://localhost:8000/crystalexplorer.html"
