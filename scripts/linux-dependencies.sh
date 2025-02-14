#!/bin/bash

# Exit on any error
set -e

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    else
        echo "unknown"
    fi
}

# Get the distribution
DISTRO=$(detect_distro)

case $DISTRO in
    "ubuntu"|"debian")
        echo "Installing dependencies for Ubuntu/Debian..."
        ;;
    "fedora")
        echo "Installing dependencies for Fedora..."
        ;;
    *)
        echo "Unsupported distribution: $DISTRO"
        exit 1
        ;;
esac
