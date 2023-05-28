#pragma once
#include "common/matrix.inl"
#include "common/mathconstants.h"

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

public:
	enum class MovementDirection
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	void Reset();

    Matrix &GetViewMatrix();
	Matrix &GetProjMatrix();
	Vector &GetLookDirection();

	void Rotate(float x, float y);
	void Move(MovementDirection direction, float distance);
};
