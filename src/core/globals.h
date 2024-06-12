#pragma once
inline const char *CE_NAME = "CrystalExplorer";
inline const char *GLOBAL_MAINWINDOW_TITLE = CE_NAME;

inline const char *CE_AUTHORS =
    "P.R. Spackman, M. J. Turner, J. J. McKinnon,<br>S. K. Wolff, D. "
    "J. Grimwood, <br>D. Jayatilaka and M. A. Spackman";
inline const char *CE_URL = "http://crystalexplorer.net";
inline const char *CE_CITATION_URL =
    "https://wiki.crystalexplorer.net/en/how-to-cite";
inline const char *CE_EMAIL = "support@crystalexplorer.net";
inline const char *COPYRIGHT_NOTICE =
    "&copy;2005-%1 University of Western Australia"; // Insert current year at
                                                     // %1
inline const char *OCC_URL = "https://github.com/peterspackman/occ";

inline const char *ANGSTROM_SYMBOL{"Å"}; // Å
inline const char *DEGREE_SYMBOL{"°"};   // °
inline const char *SQUARED_SYMBOL{"²"};  // ²
inline const char *CUBED_SYMBOL{"³"};    // ³

inline const char *ANY_ITEM = "Any"; // Used by CloseContactsDialog
// Used by CloseContactsDialog and Crystal to define the default distance
// for close contacts where X = "Any" and Y = "Any"
inline const double GLOBAL_CC_DISTANCE_CRITERIA = 3.3;

inline const int CC1_INDEX = 0;
inline const int CC2_INDEX = 1;
inline const int CC3_INDEX = 2;
inline const int CCMAX_INDEX = CC3_INDEX;

inline const double GLOBAL_FRONT_CLIPPING_PLANE = 10.0;

// When calculating close contacts we adjust the sum of the VdW radii by this
// factor so the user has access to more close contacts than just those that are
// less than the sum of the VdW radii.
// This factor should at least 1.0 because this would just be the sum of the VdW
// radii.
inline const double CLOSECONTACT_FACTOR = 1.5;

// Used by SurfaceGenerationDialog and EnergyCalculationDialog to default
// wavefunction combobox option
inline const char *NEW_WAVEFUNCTION_ITEM = "Generate New Wavefunction";
