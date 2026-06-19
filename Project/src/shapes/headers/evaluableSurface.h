#pragma once
#include <MG1Math/Vect3.h>
#include <concepts>

// C++20 Concept: Wymusza na obiekcie posiadanie funkcji potrzebnych do algorytmu Newtona
template <typename T>
concept EvaluableSurface = requires(T a, float u, float v)
{
    // Powierzchnia musi umieć zwrócić swoją pozycję w przestrzeni 3D
    { a.EvaluatePos(u, v) } -> std::same_as<Vect3>;

    // Powierzchnia musi umieć zwrócić wektory styczne
    { a.EvaluateDu(u, v) } -> std::same_as<Vect3>;
    { a.EvaluateDv(u, v) } -> std::same_as<Vect3>;

    // Powierzchnia musi określić, czy parametry się zapętlają (np. u walca/torusa)
    { a.isWrappedU() } -> std::same_as<bool>;
    { a.isWrappedV() } -> std::same_as<bool>;
};