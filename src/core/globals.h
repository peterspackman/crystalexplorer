#pragma once

namespace cx::globals {
inline constexpr const char *name = "CrystalExplorer";
inline constexpr const char *mainWindowTitle = name;

inline constexpr const char *authors =
    "P.R. Spackman, M. J. Turner, J. J. McKinnon,<br>S. K. Wolff, D. "
    "J. Grimwood, <br>D. Jayatilaka and M. A. Spackman";
inline constexpr const char *url = "https://crystalexplorer.net";
inline constexpr const char *citationUrl =
    "https://crystalexplorer.net/docs/how-to-cite.html";
inline constexpr const char *email = "support@crystalexplorer.net";
inline constexpr const char *copyrightNoticeTemplate =
    "&copy;2005-%1 CrystalExplorer authors"; // Insert current year at
                                             // %1
inline constexpr const char *occUrl = "https://github.com/peterspackman/occ";

inline constexpr const char *angstromSymbol{"Å"}; // Å
inline constexpr const char *degreeSymbol{"°"};   // °

inline constexpr const char *anyItemLabel = "Any";
inline constexpr const double frontClippingPlane = 10.0;
} // namespace cx::globals
