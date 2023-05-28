#pragma once
#include <math.h> //sqrtf, sinf, cosf

//main vector class
class __declspec(align(4)) Vector
{
public:
	float x, y, z, unused;
	
	constexpr Vector() : x(0.0f), y(0.0f), z(0.0f), unused(0.0f) {}
	constexpr Vector(float x, float y, float z, float w = 0.0f) : x(x), y(y), z(z), unused(w) {}

	operator const float *()
	{
		return &x;
	}

	operator const float *() const
	{
		return &x;
	}

	float &operator[](int index)
	{
		return (&x)[index];
	}

	const float &operator[](int index) const
	{
		return (&x)[index];
	}
	
	Vector operator+(const Vector &other) const
	{
		return Vector(x + other.x, y + other.y, z + other.z);
	}
	
	Vector operator-(const Vector &other) const
	{
		return Vector(x - other.x, y - other.y, z - other.z);
	}

	Vector operator*(const Vector &other) const
	{
		return Vector(x * other.x, y * other.y, z * other.z);
	}

	Vector operator*(float scalar) const
	{
		return Vector(x * scalar, y * scalar, z * scalar);
	}

	Vector operator-() const
	{
		return Vector(-x, -y, -z);
	}

	Vector &operator+=(const Vector &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Vector &operator-=(const Vector &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	Vector &operator*=(const Vector &other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	Vector &operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	Vector &operator/=(float scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	bool operator==(const Vector &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator!=(const Vector &other) const
	{
		return x != other.x || y != other.y || z != other.z;
	}

	float Norm() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	float LengthSquared() const
	{
		return x * x + y * y + z * z;
	}

	void Normalise()
	{
		float norm = Norm();
		x /= norm;
		y /= norm;
		z /= norm;
	}

	float Dot(const Vector &other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	Vector Cross(const Vector &other) const
	{
		return Vector(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x,
			0.0f);
	}

	Vector Lerp(const Vector &other, float t) const
	{
		return Vector(
			x + (other.x - x) * t,
			y + (other.y - y) * t,
			z + (other.z - z) * t);
	}

	Vector Slerp(const Vector &other, float t) const
	{
		float dot = Dot(other);
		float theta = acosf(dot);
		float sinTheta = sinf(theta);
		float a = sinf((1.0f - t) * theta) / sinTheta;
		float b = sinf(t * theta) / sinTheta;
		return Vector(
			x * a + other.x * b,
			y * a + other.y * b,
			z * a + other.z * b);
	}

	Vector Rotate(const Vector &axis, float angle) const
	{
		float sinAngle = sinf(angle);
		float cosAngle = cosf(angle);
		return axis * Dot(axis * (1.0f - cosAngle)) +
			(*this) * cosAngle +
			Cross(axis) * sinAngle;
	}

	Vector Rotate(const Vector &axis, const Vector &angle) const
	{
		return Rotate(axis, angle.x) * angle.y +
			Rotate(axis, angle.z);
	}

	Vector Rotate(const Vector &angle) const
	{
		return Rotate(Vector(0.0f, 0.0f, 1.0f), angle.x) * angle.y +
			Rotate(Vector(1.0f, 0.0f, 0.0f), angle.z);
	}

	Vector RotateX(float angle) const
	{
		float sinAngle = sinf(angle);
		float cosAngle = cosf(angle);
		return Vector(
			x,
			y * cosAngle - z * sinAngle,
			y * sinAngle + z * cosAngle);
	}

	Vector RotateY(float angle) const
	{
		float sinAngle = sinf(angle);
		float cosAngle = cosf(angle);
		return Vector(
			x * cosAngle + z * sinAngle,
			y,
			-x * sinAngle + z * cosAngle);
	}

	Vector RotateZ(float angle) const
	{
		float sinAngle = sinf(angle);
		float cosAngle = cosf(angle);
		return Vector(
			x * cosAngle - y * sinAngle,
			x * sinAngle + y * cosAngle,
			z);
	}
};
static_assert(sizeof(Vector) == 16, "Vector MUST be 16 bytes, while it is not!");
