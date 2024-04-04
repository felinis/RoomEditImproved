#pragma once
#include "common/matrix.inl"
#include "common/mathconstants.hh"
#include <stdint.h>

struct Viewport
{
	float x, y, width, height;

	constexpr Viewport():
		x(0.0f), y(0.0f), width(0.0f), height(0.0f)
	{}
};

class FreeLookCamera  
{
	static constexpr Vector UP = Vector(0.0f, 0.0f, 1.0f);		//+Z is up

	Vector position = Vector(0.0f, -1.0f, 0.0f);				//current position of the camera
	Vector target = Vector(0.0f, 0.0f, 0.0f);					//current target point the camera is looking at
	Vector upDirection = UP;									//current up vector (changes when the camera is rotating)
	Vector lookDirection;

	float pitch = 0.0f;
	float yaw = MathConstants::PI;

	Matrix view;
	Matrix projection;

	Viewport viewport;

public:
	enum class MovementDirection
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	void Reset();
	void ResizeViewport(uint32_t width, uint32_t height);

	const Vector &GetPosition() const;

	const Matrix &GetViewMatrix();
	const Matrix &GetProjMatrix();
	const Vector &GetLookDirection() const;

	void Rotate(float x, float y);
	void Move(MovementDirection direction, float distance);

	bool UnProjectFromScreen(const Vector &screen, Vector &world);
};
