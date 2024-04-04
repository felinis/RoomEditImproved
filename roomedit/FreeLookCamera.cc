/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "FreeLookCamera.hh"

void FreeLookCamera::Reset()
{
	//reset position and target to the origin
	position = Vector(0.0f, -1.0f, 0.0f);
	target = Vector(0.0f, 0.0f, 0.0f);

	//reset view angles
	pitch = 0.0f;
	yaw = MathConstants::PI;
}

void FreeLookCamera::ResizeViewport(uint32_t width, uint32_t height)
{
	viewport.width = (float)width;
	viewport.height = (float)height;
}

const Vector &FreeLookCamera::GetPosition() const
{
	return position;
}

const Matrix &FreeLookCamera::GetViewMatrix()
{
	lookDirection = Vector(cosf(yaw) * cosf(pitch), sinf(yaw) * cosf(pitch), sinf(pitch));
	target = position + lookDirection;
	view = Matrix::LookAtRH(position, target);
	return view;
}

const Matrix &FreeLookCamera::GetProjMatrix()
{
	projection = Matrix::PerspectiveFovRH(1.4f, viewport.width / viewport.height, 1.0f, 4096.0f * 4);
	return projection;
}

const Vector &FreeLookCamera::GetLookDirection() const
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

static bool UnProject(const Vector &screen, const Matrix &viewProjection, const Viewport &viewport, Vector &world)
{
	//transformation coordinates normalised between -1 and 1
	//	(-1, 1)--------(1, 1)
	//	|		SCREEN		|
	//	|					|
	//	(-1, -1)------(1, -1)
	Vector in(
		(screen.x - viewport.x) * 2.0f / viewport.width - 1.0f,
		(screen.y - viewport.y) * 2.0f / viewport.height - 1.0f,
		screen.z,
		1.0f
	);

	//compute inverse transformation
	Matrix inverted = viewProjection.Invert();

	//compute object coordinates
	Vector out = inverted * in;
	if (out[3] == 0.0f)
		return false;

	out /= out[3];
	world = out;
	return true;
}

bool FreeLookCamera::UnProjectFromScreen(const Vector &screen, Vector &world)
{
	return UnProject(screen, projection * view, viewport, world);
}
