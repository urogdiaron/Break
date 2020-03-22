#pragma once
using vec = sf::Vector2f;

constexpr float clamp(float value, float min, float max)
{
    return std::max(std::min(max, value), min);
}

float vec_dot(vec v1, vec v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

float vec_lengthsq(vec v)
{
    return vec_dot(v, v);
}

float vec_length(vec v)
{
    return std::sqrt(vec_lengthsq(v));
}

vec vec_normalize(vec v)
{
    return v / vec_length(v);
}

vec vec_reflect(vec v, vec normal)
{
    return v - normal * (vec_dot(v, normal) * 2.0f);
}

float lerp(float from, float to, float t)
{
    return from + t * (to - from);
}