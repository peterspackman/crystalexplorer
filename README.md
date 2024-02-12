# CrystalExplorer

CrystalExplorer is now open source, available under a LGPLv3 license terms. See COPYING.LESSER for more details.

## Status

CrystalExplorer is currently undergoing an overhaul, migrating away from using Tonto as
a backend after rewriting the rendering code.

The new features are effectively locked behind the 'Enable experimental features' flag in preferences.
So having that flag disabled *should* largely use the old code and logic.

The current state of the software should be considered unstable: many things are not working
and need fixes, and many areas of the newer implementation are not yet implemented.

In particular, saving and loading project files is quite broken at the moment due to several internal
changes and the previous reliance on binary compatibility in the internal data structures.

# Build

Building CE should be a matter of the typical CMake workflow, assuming the major dependency (Qt6) is
already installed and available.

```bash
# Assuming you're in the source directory
mkdir build && cd build

# Configure the program and library
cmake .. -GNinja

# Build the program
ninja

# Build the package (e.g. dmg file on MacOS)
ninja package
```

By default this will build the Release version. 


## Citation

See the [documentation](https://crystalexplorer.net/docs/how-to-cite), but for basic functionality please cite the following:

```bibtex
@article{Spackman:oc5008,
author = "Spackman, Peter R. and Turner, Michael J. and McKinnon, Joshua J. and Wolff, Stephen K. and Grimwood, Daniel J. and Jayatilaka, Dylan and Spackman, Mark A.",
title = "{{\it CrystalExplorer}: a program for Hirshfeld surface analysis, visualization and quantitative analysis of molecular crystals}",
journal = "Journal of Applied Crystallography",
year = "2021",
volume = "54",
number = "3",
pages = "",
month = "Jun",
doi = {10.1107/S1600576721002910},
url = {https://doi.org/10.1107/S1600576721002910},
}
```
