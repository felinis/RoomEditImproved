#pragma once
#include "vector.inl"
#include <xmmintrin.h>
#include <smmintrin.h>

//we are using a right-handed coordinate system with Z up (like Blender)
static constexpr Vector UPVECTOR = { 0.0f, 0.0f, 1.0f, 0.0f };

class __declspec(align(16)) Matrix
{
	Vector rows[4];
	
public:
	//default constructor for the matrix is an identity matrix
	constexpr Matrix()
	{
		rows[0] = { 1.0f, 0.0f, 0.0f, 0.0f };
		rows[1] = { 0.0f, 1.0f, 0.0f, 0.0f };
		rows[2] = { 0.0f, 0.0f, 1.0f, 0.0f };
		rows[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
	}

	operator float *()
	{
		return &rows[0].x;
	}

	operator const float *() const
	{
		return &rows[0].x;
	}

	Vector &operator[](int i)
	{
		return rows[i];
	}

	const Vector &operator[](int i) const
	{
		return rows[i];
	}

	const Vector &GetTranslation() const
	{
		return rows[3];
	}

	void SetTranslation(const Vector &translation)
	{
		rows[3] = { translation.x, translation.y, translation.z, 1.0f };
	}

	//https://www.cs.helsinki.fi/u/ilmarihe
	static void mmul_sse(const float *a, const float *b, float *r)
	{
		__m128 a_line, b_line, r_line;
		for (int i = 0; i < 16; i += 4) {
			// unroll the first step of the loop to avoid having to initialize r_line to zero
			a_line = _mm_load_ps(a);         // a_line = vec4(column(a, 0))
			b_line = _mm_set1_ps(b[i]);      // b_line = vec4(b[i][0])
			r_line = _mm_mul_ps(a_line, b_line); // r_line = a_line * b_line
			for (int j = 1; j < 4; j++) {
				a_line = _mm_load_ps(&a[j * 4]); // a_line = vec4(column(a, j))
				b_line = _mm_set1_ps(b[i + j]);  // b_line = vec4(b[i][j])
				// r_line += a_line * b_line
				r_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), r_line);
			}
			_mm_store_ps(&r[i], r_line);     // r[i] = r_line
		}
	}

	//multiply two matrices using the * operator
	Matrix operator*(const Matrix &other) const
	{
#if 0
		Matrix result;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.rows[i][j] = rows[i][0] * other.rows[0][j] + rows[i][1] * other.rows[1][j] + rows[i][2] * other.rows[2][j] + rows[i][3] * other.rows[3][j];
			}
		}
		return result;
#else
		Matrix out;
		mmul_sse(&rows[0].x, &other.rows[0].x, &out.rows[0].x);
		return out;
#endif
	}

	//perform a matrix-vector multiplication
	Vector operator*(const Vector &v) const
	{
		Vector result;
		for (int i = 0; i < 4; i++)
		{
			result[i] = rows[i][0] * v[0] + rows[i][1] * v[1] + rows[i][2] * v[2] + rows[i][3] * v[3];
		}
		return result;
	}

	void RotateZ(float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);
		rows[0] = Vector(c, s, 0.0f, 0.0f);
		rows[1] = Vector(-s, c, 0.0f, 0.0f);
		rows[2] = Vector(0.0f, 0.0f, 1.0f, 0.0f);
	}

	//transpose a matrix
#if 0
	Matrix Transpose() const
	{
		Matrix result;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.rows[i][j] = rows[j][i];
			}
		}
		return result;
	}
#endif

	//create a right-handed perspective matrix
	static Matrix PerspectiveFovRH(float vertical_fov, float aspect, float zNear, float zFar)
	{
		float Height = 1.0f / tanf(0.5f * vertical_fov);
		float Width = Height / aspect;
		float fRange = zFar / (zNear - zFar);

		Matrix m;
		m[0] = Vector(Width, 0.0f, 0.0f, 0.0f);
		m[1] = Vector(0.0f, Height, 0.0f, 0.0f);
		m[2] = Vector(0.0f, 0.0f, fRange, -1.0f);
		m[3] = Vector(0.0f, 0.0f, fRange * zNear, 0.0f);
		return m;
	}
#if 1
	static Matrix LookAt(const Vector &eye, const Vector &forwardDirection, const Vector &up)
	{
		Vector rightDirection = forwardDirection.Cross(up);
		rightDirection.Normalise();
		Vector upDirection = rightDirection.Cross(forwardDirection);
		upDirection.Normalise();

		Matrix result;
		result[0] = Vector(rightDirection.x, upDirection.x, forwardDirection.x);
		result[1] = Vector(rightDirection.y, upDirection.y, forwardDirection.y);
		result[2] = Vector(rightDirection.z, upDirection.z, forwardDirection.z);
		result[3] = Vector(-rightDirection.Dot(eye), -upDirection.Dot(eye), -forwardDirection.Dot(eye), 1.0f);
		return result;
	}
#else
	static Matrix LookAt(const Vector &eye, const Vector &forwardDirection, const Vector &up)
	{
		Vector rightDirection = up.Cross(forwardDirection);
		rightDirection.Normalise();
		Vector upDirection = forwardDirection.Cross(rightDirection);
		upDirection.Normalise();

		Matrix result;
		result[0] = Vector(rightDirection.x, upDirection.x, forwardDirection.x);
		result[1] = Vector(rightDirection.y, upDirection.y, forwardDirection.y);
		result[2] = Vector(rightDirection.z, upDirection.z, forwardDirection.z);
		result[3] = Vector(-rightDirection.Dot(eye), -upDirection.Dot(eye), -forwardDirection.Dot(eye), 1.0f);
		return result;
	}
#endif
	//create a right-handed view matrix
	static Matrix LookAtRH(const Vector &eye, const Vector &target)
	{
		Vector direction = eye - target;
		direction.Normalise();
		return LookAt(eye, direction, UPVECTOR);
	}
};
static_assert(sizeof(Matrix) == 64, "Matrix MUST be 64 bytes, while it is not!");
