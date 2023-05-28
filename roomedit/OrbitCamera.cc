#include "OrbitCamera.h"

Matrix &OrbitCamera::GetViewMatrix()
{
	view = Matrix::LookAtRH(position, target);
//	view = Matrix::LookAtRH(Vector(0.0f, -10.0f, 0.0f), Vector());
	return view;
}

Matrix &OrbitCamera::GetProjMatrix()
{
	projection = Matrix::PerspectiveFovRH(MathConstants::PI / 4.0f, 800.0f / 600.0f, 0.1f, 4096.0f);
	return projection;
}

Vector &OrbitCamera::GetLookDirection()
{
	lookDirection = position - target;
	return lookDirection;
}

void OrbitCamera::Rotate(float x, float y)
{
	//yaw is the left-right rotation, pitch is the up-down
//	yaw -= x * 0.1f;
//	pitch += y * 0.1f;

	//clamp pitch to avoid gimbal lock
//	pitch = fmaxf(fminf(pitch, MathConstants::PI / 2.0f - 0.01f), -MathConstants::PI / 2.0f + 0.01f);

	//calculate the view vector
	Vector viewVector = position - target;
	float radius = viewVector.Norm();

	//convert view vector to spherical coordinates
	float theta = atan2f(viewVector.y, viewVector.x);
	float phi = acosf(viewVector.z / radius);

	//update the angles based on the pitch and yaw
	theta -= x;
	phi -= y;

	//clamp phi to avoid gimbal lock
	phi = fmaxf(fminf(phi, MathConstants::PI - 0.01f), 0.01f);

	//convert the updated spherical coordinates back to Cartesian coordinates
	position.x = target.x + radius * sinf(phi) * cosf(theta);
	position.y = target.y + radius * sinf(phi) * sinf(theta);
	position.z = target.z + radius * cosf(phi);

	// Update the up vector
	Vector forward = target - position;
	Vector right = forward.Cross(UPVECTOR);
	right.Normalise();
	upDirection = right.Cross(forward);
	upDirection.Normalise();
}

void OrbitCamera::Pan(float x, float y)
{
	Vector rightDirection = Vector(target - position).Cross(upDirection);
	rightDirection.Normalise();

	position -= rightDirection * x;
	position += upDirection * y;
	target -= rightDirection * x;
	target += upDirection * y;
}

void OrbitCamera::Zoom(float zoomFactor)
{
	Vector camPos = position;
	Vector camDir = target - camPos;
	camDir.Normalise();

	float distance = Vector(target - camPos).Norm();
	Vector newPos = camPos + camDir * (zoomFactor * distance);

	position = newPos;
	this->zoomFactor = zoomFactor;
}
