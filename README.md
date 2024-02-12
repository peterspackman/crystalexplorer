# CrystalExplorer

CrystalExplorer is now open source, available under a LGPLv3 license terms. See COPYING.LESSER for more details.

## Status

| Operating System | Qt Version | Build status | Automatic deployment |
|------------------|------------|--------------|----------------------|
| OS X | 5.5.1 (Homebrew) | :white_check_mark: | :white_check_mark: |
| Linux (Ubuntu 14.04) | 5.2.1 | :white_check_mark: | :white_check_mark: |
| Windows | 5.5.1 | :white_check_mark: | :negative_squared_cross_mark: |

# Build

Assuming tonto is built (statically linked) and located at `~tonto/run_molecule`
```bash
# Copy resources over from tonto build
cp -r ~/tonto/basis_sets resources
cp ~/tonto/run_molecule resources/tonto #tonto.exe on windows
# Make build directory and run
mkdir build && cd build 
cmake ..
make # can use -j flag or specify -GNinja as build and run with ninja
```
By default this will build the Release version. 

## Deployment 
* *OSX:* `cpack` generates a `.dmg` with all required dependencies bundled into the `.app` container.
* *Linux:* `cpack' generates a `.deb` with required dependencies specified (all standard ubuntu packages).
* *Windows:* `cpack` currently not working, planning to use NSIS installer.
