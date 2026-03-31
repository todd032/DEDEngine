#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace cotrx
{
constexpr float Pi = 3.14159265358979323846f;

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Color
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

inline Vec2 operator+(const Vec2& left, const Vec2& right)
{
    return {left.x + right.x, left.y + right.y};
}

inline Vec2 operator-(const Vec2& left, const Vec2& right)
{
    return {left.x - right.x, left.y - right.y};
}

inline Vec2 operator*(const Vec2& value, float scalar)
{
    return {value.x * scalar, value.y * scalar};
}

inline Vec3 operator+(const Vec3& left, const Vec3& right)
{
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

inline Vec3 operator-(const Vec3& left, const Vec3& right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

inline Vec3 operator*(const Vec3& value, float scalar)
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

inline Vec3 operator*(float scalar, const Vec3& value)
{
    return value * scalar;
}

inline Vec3 operator/(const Vec3& value, float scalar)
{
    return {value.x / scalar, value.y / scalar, value.z / scalar};
}

inline Vec3& operator+=(Vec3& left, const Vec3& right)
{
    left.x += right.x;
    left.y += right.y;
    left.z += right.z;
    return left;
}

inline Vec3& operator-=(Vec3& left, const Vec3& right)
{
    left.x -= right.x;
    left.y -= right.y;
    left.z -= right.z;
    return left;
}

inline float Dot(const Vec3& left, const Vec3& right)
{
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

inline Vec3 Cross(const Vec3& left, const Vec3& right)
{
    return {
        (left.y * right.z) - (left.z * right.y),
        (left.z * right.x) - (left.x * right.z),
        (left.x * right.y) - (left.y * right.x)};
}

inline float LengthSquared(const Vec3& value)
{
    return Dot(value, value);
}

inline float Length(const Vec3& value)
{
    return std::sqrt(LengthSquared(value));
}

inline Vec3 Normalize(const Vec3& value)
{
    const auto length = Length(value);
    if (length <= 1.0e-6f)
    {
        return {0.0f, 1.0f, 0.0f};
    }

    return value / length;
}

inline Vec3 Lerp(const Vec3& start, const Vec3& end, float t)
{
    return start + ((end - start) * t);
}

inline float Lerp(float start, float end, float t)
{
    return start + ((end - start) * t);
}

inline float Clamp(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

inline float ToRadians(float degrees)
{
    return degrees * Pi / 180.0f;
}

inline float ToDegrees(float radians)
{
    return radians * 180.0f / Pi;
}

struct Mat4
{
    std::array<float, 16> m{};

    static Mat4 Identity()
    {
        Mat4 matrix{};
        matrix.m = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
        return matrix;
    }

    static Mat4 Translation(const Vec3& value)
    {
        auto matrix = Identity();
        matrix.m[12] = value.x;
        matrix.m[13] = value.y;
        matrix.m[14] = value.z;
        return matrix;
    }

    static Mat4 Basis(const Vec3& xAxis, const Vec3& yAxis, const Vec3& zAxis)
    {
        Mat4 matrix{};
        matrix.m = {
            xAxis.x, xAxis.y, xAxis.z, 0.0f,
            yAxis.x, yAxis.y, yAxis.z, 0.0f,
            zAxis.x, zAxis.y, zAxis.z, 0.0f,
            0.0f,    0.0f,    0.0f,    1.0f};
        return matrix;
    }

    static Mat4 Perspective(float fieldOfViewRadians, float aspect, float nearPlane, float farPlane)
    {
        const auto f = 1.0f / std::tan(fieldOfViewRadians * 0.5f);
        Mat4 matrix{};
        matrix.m = {
            f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f, f, 0.0f, 0.0f,
            0.0f, 0.0f, (farPlane + nearPlane) / (nearPlane - farPlane), -1.0f,
            0.0f, 0.0f, (2.0f * farPlane * nearPlane) / (nearPlane - farPlane), 0.0f};
        return matrix;
    }

    static Mat4 Orthographic(float left, float right, float bottom, float top)
    {
        Mat4 matrix = Identity();
        matrix.m[0] = 2.0f / (right - left);
        matrix.m[5] = 2.0f / (top - bottom);
        matrix.m[10] = -1.0f;
        matrix.m[12] = -(right + left) / (right - left);
        matrix.m[13] = -(top + bottom) / (top - bottom);
        return matrix;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
    {
        const auto forward = Normalize(target - eye);
        const auto right = Normalize(Cross(forward, up));
        const auto actualUp = Normalize(Cross(right, forward));

        Mat4 matrix{};
        matrix.m = {
            right.x, actualUp.x, -forward.x, 0.0f,
            right.y, actualUp.y, -forward.y, 0.0f,
            right.z, actualUp.z, -forward.z, 0.0f,
            -Dot(right, eye), -Dot(actualUp, eye), Dot(forward, eye), 1.0f};
        return matrix;
    }
};

inline Mat4 operator*(const Mat4& left, const Mat4& right)
{
    Mat4 product{};
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            product.m[(column * 4) + row] =
                (left.m[(0 * 4) + row] * right.m[(column * 4) + 0]) +
                (left.m[(1 * 4) + row] * right.m[(column * 4) + 1]) +
                (left.m[(2 * 4) + row] * right.m[(column * 4) + 2]) +
                (left.m[(3 * 4) + row] * right.m[(column * 4) + 3]);
        }
    }

    return product;
}

struct Bounds
{
    Vec3 minimum{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    Vec3 maximum{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};
};

inline void ExpandBounds(Bounds& bounds, const Vec3& point)
{
    bounds.minimum.x = std::min(bounds.minimum.x, point.x);
    bounds.minimum.y = std::min(bounds.minimum.y, point.y);
    bounds.minimum.z = std::min(bounds.minimum.z, point.z);
    bounds.maximum.x = std::max(bounds.maximum.x, point.x);
    bounds.maximum.y = std::max(bounds.maximum.y, point.y);
    bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

struct Vertex
{
    Vec3 position;
    Vec3 normal;
    Color color;
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    Bounds bounds;
};

struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 xAxis{1.0f, 0.0f, 0.0f};
    Vec3 yAxis{0.0f, 1.0f, 0.0f};
    Vec3 zAxis{0.0f, 0.0f, 1.0f};

    [[nodiscard]] Mat4 ToMatrix() const
    {
        auto basis = Mat4::Basis(xAxis, yAxis, zAxis);
        basis.m[12] = position.x;
        basis.m[13] = position.y;
        basis.m[14] = position.z;
        return basis;
    }
};

struct EngineConfig
{
    int windowWidth = 1360;
    int windowHeight = 820;
    int targetFps = 60;
    std::string title = "Cookie On The Roof X";
    Color clearColor{0.95f, 0.95f, 0.92f, 1.0f};
    bool preferGles = false;
    bool landscapeOnly = false;
};

struct OrbitCamera
{
    Vec3 target{0.0f, 4.5f, 0.0f};
    float yaw = -0.72f;
    float pitch = 0.48f;
    float distance = 18.5f;
    float minPitch = ToRadians(10.0f);
    float maxPitch = ToRadians(60.0f);
    float minDistance = 10.0f;
    float maxDistance = 30.0f;
    float fieldOfView = ToRadians(48.0f);

    void Orbit(float deltaYaw, float deltaPitch)
    {
        yaw -= deltaYaw;
        pitch = Clamp(pitch - deltaPitch, minPitch, maxPitch);
    }

    void Zoom(float deltaDistance)
    {
        distance = Clamp(distance + deltaDistance, minDistance, maxDistance);
    }

    [[nodiscard]] Vec3 Position() const
    {
        const auto cosPitch = std::cos(pitch);
        return {
            target.x + (std::sin(yaw) * cosPitch * distance),
            target.y + (std::sin(pitch) * distance),
            target.z + (std::cos(yaw) * cosPitch * distance)};
    }

    [[nodiscard]] Mat4 ViewMatrix() const
    {
        return Mat4::LookAt(Position(), target, {0.0f, 1.0f, 0.0f});
    }

    [[nodiscard]] Mat4 ProjectionMatrix(float aspect) const
    {
        return Mat4::Perspective(fieldOfView, aspect, 0.1f, 180.0f);
    }
};

struct SceneObject
{
    std::size_t meshIndex = 0;
    Transform transform{};
    Color tint{1.0f, 1.0f, 1.0f, 1.0f};
};

enum class UiAction : std::uint8_t
{
    None = 0,
    RandomizeRoof,
    RedistributeCookies,
};

struct ButtonState
{
    UiAction action = UiAction::None;
    std::string label;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool hovered = false;
    bool pressed = false;

    [[nodiscard]] bool Contains(float px, float py) const
    {
        return px >= x && py >= y && px <= (x + width) && py <= (y + height);
    }
};

struct UiOverlayState
{
    std::vector<ButtonState> buttons;
    std::vector<std::string> hudLines;
    std::string footer;
    std::string versionLabel;
};
} // namespace cotrx
