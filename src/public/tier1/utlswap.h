//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: defines copy&swap semantics for template library classes
//
//=============================================================================

#ifndef UTLSWAP_H
#define UTLSWAP_H

/// Remove a reference
template<typename T>
struct CUtlRemoveReference {
    typedef typename T Type;
};

template<typename T>
struct CUtlRemoveReference<T&> {
    typedef typename T Type;
};

template<typename T>
struct CUtlRemoveReference<T&&> {
    typedef typename T Type;
};

/// A black-box that ensures the provided argument is returned as an rvalue reference
/// Move operators only need to ensure the original copy is assignable and destructable,
/// while copy operators are more strict: both the original and clone must be valid for all operations.
template<typename T>
inline typename CUtlRemoveReference<T>::Type&& CUtlMove(T&& arg) {
    return static_cast<typename CUtlRemoveReference<T>::Type&&>(arg);
}

/// Swap two objects using their move assignment operator.
template<typename T>
inline void CUtlSwap(T& a, T& b) {
    T c = CUtlMove(a);
    a = CUtlMove(b);
    b = CUtlMove(c);
}

#endif //UTLSWAP_H
