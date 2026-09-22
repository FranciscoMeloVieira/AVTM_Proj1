#pragma once

#include <cmath>
#include <initializer_list>

struct Vector3
{
    float x;
    float y;
    float z;

    Vector3()
        : x(0.0f), y(0.0f), z(0.0f)
    {
    }

    Vector3(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }

    // Convert from float[3]
    Vector3(const float v[3])
        : x(v[0]), y(v[1]), z(v[2])
    {
    }

    // Convert to float[3]
    void toArray(float out[3]) const
    {
        out[0] = x;
        out[1] = y;
        out[2] = z;
    }

    Vector3& operator=(std::initializer_list<float> values)
    {
        auto it = values.begin();

        x = *it++;
        y = *it++;
        z = *it++;

        return *this;
    }

    // Basic arithmetic
    Vector3 operator+(const Vector3& other) const
    {
        return Vector3(
            x + other.x,
            y + other.y,
            z + other.z
        );
    }

    Vector3 operator-(const Vector3& other) const
    {
        return Vector3(
            x - other.x,
            y - other.y,
            z - other.z
        );
    }

    Vector3 operator-() const
    {
        return {
            -x,
            -y,
            -z
        };
    }

    Vector3 operator*(float scalar) const
    {
        return Vector3(
            x * scalar,
            y * scalar,
            z * scalar
        );
    }

    Vector3 operator/(float scalar) const
    {
        return Vector3(
            x / scalar,
            y / scalar,
            z / scalar
        );
    }

    Vector3& operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }
    
};

// scalar * vector
inline Vector3 operator*(float scalar, const Vector3& v)
{
    return v * scalar;
}

inline float Dot(const Vector3& a, const Vector3& b)
{
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z;
}

inline Vector3 Cross(const Vector3& a, const Vector3& b)
{
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline float LengthSquared(const Vector3& v)
{
    return Dot(v, v);
}

inline float Length(const Vector3& v)
{
    return std::sqrt(LengthSquared(v));
}

inline Vector3 Normalize(const Vector3& v)
{
    float length = Length(v);

    if (length <= 0.000001f)
        return Vector3();

    return v / length;
}

inline float Distance(const Vector3& a, const Vector3& b)
{
    return Length(a - b);
}

inline Vector3 RotateY(const Vector3& v, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    return {
        v.x * c - v.z * s,
        v.y,
        v.x * s + v.z * c
    };
}

inline Vector3 Lerp(
    const Vector3& a,
    const Vector3& b,
    float t)
{
    return a + (b - a) * t;
}