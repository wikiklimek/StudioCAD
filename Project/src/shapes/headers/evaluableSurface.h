#pragma once
#include <MG1Math/Vect3.h>
#include <concepts>


template <typename T>
concept EvaluableSurface = requires(T a, float u, float v)
{
    { a.EvaluatePos(u, v) } -> std::same_as<Vect3>;

    { a.EvaluateDu(u, v) } -> std::same_as<Vect3>;
    { a.EvaluateDv(u, v) } -> std::same_as<Vect3>;

    { a.isWrappedU() } -> std::same_as<bool>;
    { a.isWrappedV() } -> std::same_as<bool>;
};