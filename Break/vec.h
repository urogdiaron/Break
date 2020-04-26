#pragma once
using vec = sf::Vector2f;

bool vec_is_zero(vec v)
{
    return v.x == 0 && v.y == 0;
}

template<class T>
constexpr T clamp(T value, T min, T max)
{
    return std::max(std::min(max, value), min);
}

float vec_dot(vec v1, vec v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

bool vec_ccw(vec v1, vec v2)
{
    float crossProductZ = v1.x * v2.y - v2.x * v1.y;
    return crossProductZ >= 0;
}

vec vec_rotate(vec v, float angleRadian)
{
    float c = cosf(angleRadian);
    float s = sinf(angleRadian);

    vec ret;
    ret.x = c * v.x - s * v.y;
    ret.y = s * v.x + c * v.y;
    return ret;
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