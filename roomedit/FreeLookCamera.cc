#include "FreeLookCamera.h"

void FreeLookCamera::Reset()
{
	//reset position and target to the origin
	position = Vector(0.0f, -1.0f, 0.0f);
	target = Vector(0.0f, 0.0f, 0.0f);

	//reset view angles
	pitch = 0.0f;
	yaw = MathConstants::PI;
}

Matrix &FreeLookCamera::GetViewMatrix()
{
	lookDirection = Vector(cosf(yaw) * cosf(pitch), sinf(yaw) * cosf(pitch), sinf(pitch));
	target = position + lookDirection;
	view = Matrix::LookAtRH(position, target);
	return view;
}

Matrix &FreeLookCamera::GetProjMatrix()
{
	projection = Matrix::PerspectiveFovRH(MathConstants::PI / 2.5f, 800.0f / 600.0f, 1.0f, 4096.0f * 4);
	return projection;
}

Vector &FreeLookCamera::GetLookDirection()
{
//	lookDirection = position - target;
	return lookDirection;
}

void FreeLookCamera::Rotate(float x, float y)
{
	//yaw is the left-right rotation, pitch is the up-down
	yaw += x;
	pitch -= y;

	//clamp pitch to avoid gimbal lock
	float pitchLimit = MathConstants::PI / 2.0f - 0.01f;
	pitch = fmaxf(-pitchLimit, fminf(pitchLimit, pitch));
}

void FreeLookCamera::Move(MovementDirection direction, float distance)
{
	Vector moveVector;

	switch (direction)
	{
	case MovementDirection::FORWARD:
		moveVector = lookDirection * distance;
		break;
	case MovementDirection::BACKWARD:
		moveVector = lookDirection * -distance;
		break;
	case MovementDirection::LEFT:
		moveVector = lookDirection.Cross(upDirection);
		moveVector.Normalise();
		moveVector *= distance;
		break;
	case MovementDirection::RIGHT:
		moveVector = lookDirection.Cross(upDirection);
		moveVector.Normalise();
		moveVector *= -distance;
		break;
	default:
		break;
	}

	position += moveVector;
//	target += moveVector;

	//update the view matrix after changing the camera position
//	view = Matrix::LookAtRH(position, position + lookDirection);
}
