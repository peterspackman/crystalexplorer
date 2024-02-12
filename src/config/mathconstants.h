#pragma once
#ifndef WIN32
namespace Constants {
template <class T>
constexpr T
    pi = T(3.141592653589793238462643383279502884197169399375105820974944);
template <class T>
constexpr T two_pi = T(6.2831853071795864769252867665590057683943388015061);
template <class T>
constexpr T sqrt_pi = T(1.7724538509055160272981674833411451827975);
template <class T>
constexpr T sqrt_half_pi = T(1.253314137315500251207882642405522626503);
template <class T>
constexpr T sqrt_two_pi = T(2.506628274631000502415765284811045253007);
template <class T>
constexpr T
    e = T(2.7182818284590452353602874713526624977572470936999595749669676);
template <class T>
constexpr T angstroms_per_bohr = T(0.52917721092000002923994491662820121095);
template <class T>
constexpr T bohr_per_angstrom = T(1.889726124565061881906545595284207);
template <class T>
constexpr T kj_per_kcal = T(0.2390057361376673040152963671128107);
template <class T>
constexpr T kcal_per_kj = T(4.18400000000000000000000000000000000);
template <class T> constexpr T rad_per_deg = T(pi<T> / 180.0);
template <class T> constexpr T deg_per_rad = T(180.0 / pi<T>);
template <class T> constexpr T kjmol_per_hartree = T(2625.5002);
template <class T> constexpr T hartree_per_kjmol = T(0.00038087980339899);
} // namespace Constants
inline const float PI = Constants::pi<float>;
inline const float ANGSTROM_PER_BOHR = Constants::angstroms_per_bohr<float>;
inline const float BOHR_PER_ANGSTROM = Constants::bohr_per_angstrom<float>;
inline const float RAD_PER_DEG = Constants::rad_per_deg<float>;
inline const float DEG_PER_RAD = Constants::deg_per_rad<float>;
inline const float KCAL_PER_MOL_TO_KJ_PER_MOL = Constants::kcal_per_kj<float>;
inline const float KJMOL_PER_HARTREE = Constants::kjmol_per_hartree<float>;

#else

inline const float PI =
    3.141592653589793238462643383279502884197169399375105820974944f;
inline const float TWO_PI =
    6.2831853071795864769252867665590057683943388015061f;
inline const float SQRT_PI = 1.7724538509055160272981674833411451827975f;
inline const float SQRT_HALF_PI = 1.253314137315500251207882642405522626503f;
inline const float SQRT_TWO_PI = 2.506628274631000502415765284811045253007f;
inline const float E =
    2.7182818284590452353602874713526624977572470936999595749669676f;
inline const float ANGSTROM_PER_BOHR =
    0.52917721092000002923994491662820121095f;
inline const float BOHR_PER_ANGSTROM = 1.889726124565061881906545595284207f;
inline const float KCAL_PER_MOL_TO_KJ_PER_MOL =
    0.2390057361376673040152963671128107f;
inline const float KJ_PER_MOL_TO_KCAL_PER_MOL =
    4.18400000000000000000000000000000000f;
inline const float RAD_PER_DEG = PI / 180.0f;
inline const float DEG_PER_RAD = 180.0f / PI;
inline const float KJMOL_PER_HARTREE = 2625.5002f;
#endif
inline const float EPSILON = 0.000001f;
