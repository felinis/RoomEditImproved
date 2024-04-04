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

	void SetRotationXYZ(const Vector &rotation)
	{
		//TODO!
	}

	void SetScale(float scale)
	{
		rows[0].x = scale;
		rows[1].y = scale;
		rows[2].z = scale;
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

	//perform a vector-matrix multiplication
	//WARNING: this assumes a column-major format
	Vector operator*(const Vector &v) const
	{
		Vector result;
		for (int i = 0; i < 4; i++)
		{
			result[i] =
				rows[i].x * v.x +
				rows[i].y * v.y +
				rows[i].z * v.z +
				rows[i].w * v.w;
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
#if 1
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

	//the following code was borrowed from CGLM
	typedef Matrix mat4;

#if defined(_MSC_VER) && !defined(__FMA__) && defined(__AVX2__)
#  define __FMA__ 1
#endif

#  define glmm_load(p)      _mm_load_ps(p)
#  define glmm_store(p, a)  _mm_store_ps(p, a)

#define glmm_set1(x) _mm_set1_ps(x)
#define glmm_128     __m128

#ifdef CGLM_USE_INT_DOMAIN
#  define glmm_shuff1(xmm, z, y, x, w)                                        \
     _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm),                \
                                        _MM_SHUFFLE(z, y, x, w)))
#else
#  define glmm_shuff1(xmm, z, y, x, w)                                        \
       _mm_shuffle_ps(xmm, xmm, _MM_SHUFFLE(z, y, x, w))
#endif

#define glmm_splat(x, lane) glmm_shuff1(x, lane, lane, lane, lane)

#define glmm_splat_x(x) glmm_splat(x, 0)
#define glmm_splat_y(x) glmm_splat(x, 1)
#define glmm_splat_z(x) glmm_splat(x, 2)
#define glmm_splat_w(x) glmm_splat(x, 3)

	/* glmm_shuff1x() is DEPRECATED!, use glmm_splat() */
#define glmm_shuff1x(xmm, x) glmm_shuff1(xmm, x, x, x, x)

#define glmm_shuff2(a, b, z0, y0, x0, w0, z1, y1, x1, w1)                     \
     glmm_shuff1(_mm_shuffle_ps(a, b, _MM_SHUFFLE(z0, y0, x0, w0)),           \
                 z1, y1, x1, w1)

	/* Note that `0x80000000` corresponds to `INT_MIN` for a 32-bit int. */
#define GLMM_NEGZEROf ((int)0x80000000) /*  0x80000000 ---> -0.0f  */

#define GLMM__SIGNMASKf(X, Y, Z, W)                                           \
   _mm_castsi128_ps(_mm_set_epi32(X, Y, Z, W))
  /* _mm_set_ps(X, Y, Z, W); */

#define glmm_float32x4_SIGNMASK_NPNP GLMM__SIGNMASKf(GLMM_NEGZEROf, 0, GLMM_NEGZEROf, 0)

	static inline
		__m128
		glmm_fmadd(__m128 a, __m128 b, __m128 c)
	{
#ifdef __FMA__
		return _mm_fmadd_ps(a, b, c);
#else
		return _mm_add_ps(c, _mm_mul_ps(a, b));
#endif
	}

	static inline
		__m128
		glmm_fnmadd(__m128 a, __m128 b, __m128 c)
	{
#ifdef __FMA__
		return _mm_fnmadd_ps(a, b, c);
#else
		return _mm_sub_ps(c, _mm_mul_ps(a, b));
#endif
	}

	static inline
		__m128
		glmm_vhadd(__m128 v)
	{
		__m128 x0;
		x0 = _mm_add_ps(v, glmm_shuff1(v, 0, 1, 2, 3));
		x0 = _mm_add_ps(x0, glmm_shuff1(x0, 1, 0, 0, 1));
		return x0;
	}

	static void glm_mat4_scale_p(mat4 &m, float s)
	{
		m[0][0] *= s; m[0][1] *= s; m[0][2] *= s; m[0][3] *= s;
		m[1][0] *= s; m[1][1] *= s; m[1][2] *= s; m[1][3] *= s;
		m[2][0] *= s; m[2][1] *= s; m[2][2] *= s; m[2][3] *= s;
		m[3][0] *= s; m[3][1] *= s; m[3][2] *= s; m[3][3] *= s;
	}

	//code comes from https://github.com/recp/cglm/blob/master/include/cglm/simd/sse2/mat4.h
	//original name was "glm_mat4_inv_sse2"
	Matrix Invert() const
	{
#if 1
		Matrix tra = Transpose(); //we need to transpose since we're using row-major

		__m128 r0, r1, r2, r3,
			v0, v1, v2, v3,
			t0, t1, t2, t3, t4, t5,
			x0, x1, x2, x3, x4, x5, x6, x7, x8, x9;

		x8 = glmm_float32x4_SIGNMASK_NPNP;
		x9 = glmm_shuff1(x8, 2, 1, 2, 1);

		/* 127 <- 0 */
		r0 = glmm_load(tra[0]); /* d c b a */
		r1 = glmm_load(tra[1]); /* h g f e */
		r2 = glmm_load(tra[2]); /* l k j i */
		r3 = glmm_load(tra[3]); /* p o n m */

		x0 = _mm_movehl_ps(r3, r2);                            /* p o l k */
		x3 = _mm_movelh_ps(r2, r3);                            /* n m j i */
		x1 = glmm_shuff1(x0, 1, 3, 3, 3);                      /* l p p p */
		x2 = glmm_shuff1(x0, 0, 2, 2, 2);                      /* k o o o */
		x4 = glmm_shuff1(x3, 1, 3, 3, 3);                      /* j n n n */
		x7 = glmm_shuff1(x3, 0, 2, 2, 2);                      /* i m m m */

		x6 = _mm_shuffle_ps(r2, r1, _MM_SHUFFLE(0, 0, 0, 0));  /* e e i i */
		x5 = _mm_shuffle_ps(r2, r1, _MM_SHUFFLE(1, 1, 1, 1));  /* f f j j */
		x3 = _mm_shuffle_ps(r2, r1, _MM_SHUFFLE(2, 2, 2, 2));  /* g g k k */
		x0 = _mm_shuffle_ps(r2, r1, _MM_SHUFFLE(3, 3, 3, 3));  /* h h l l */

		t0 = _mm_mul_ps(x3, x1);
		t1 = _mm_mul_ps(x5, x1);
		t2 = _mm_mul_ps(x5, x2);
		t3 = _mm_mul_ps(x6, x1);
		t4 = _mm_mul_ps(x6, x2);
		t5 = _mm_mul_ps(x6, x4);

		t0 = glmm_fnmadd(x2, x0, t0);
		t1 = glmm_fnmadd(x4, x0, t1);
		t2 = glmm_fnmadd(x4, x3, t2);
		t3 = glmm_fnmadd(x7, x0, t3);
		t4 = glmm_fnmadd(x7, x3, t4);
		t5 = glmm_fnmadd(x7, x5, t5);

		x4 = _mm_movelh_ps(r0, r1);        /* f e b a */
		x5 = _mm_movehl_ps(r1, r0);        /* h g d c */

		x0 = glmm_shuff1(x4, 0, 0, 0, 2);  /* a a a e */
		x1 = glmm_shuff1(x4, 1, 1, 1, 3);  /* b b b f */
		x2 = glmm_shuff1(x5, 0, 0, 0, 2);  /* c c c g */
		x3 = glmm_shuff1(x5, 1, 1, 1, 3);  /* d d d h */

		v2 = _mm_mul_ps(x0, t1);
		v1 = _mm_mul_ps(x0, t0);
		v3 = _mm_mul_ps(x0, t2);
		v0 = _mm_mul_ps(x1, t0);

		v2 = glmm_fnmadd(x1, t3, v2);
		v3 = glmm_fnmadd(x1, t4, v3);
		v0 = glmm_fnmadd(x2, t1, v0);
		v1 = glmm_fnmadd(x2, t3, v1);

		v3 = glmm_fmadd(x2, t5, v3);
		v0 = glmm_fmadd(x3, t2, v0);
		v2 = glmm_fmadd(x3, t5, v2);
		v1 = glmm_fmadd(x3, t4, v1);

		v0 = _mm_xor_ps(v0, x8);
		v2 = _mm_xor_ps(v2, x8);
		v1 = _mm_xor_ps(v1, x9);
		v3 = _mm_xor_ps(v3, x9);

		/* determinant */
		x0 = _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(0, 0, 0, 0));
		x1 = _mm_shuffle_ps(v2, v3, _MM_SHUFFLE(0, 0, 0, 0));
		x0 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(2, 0, 2, 0));

		x0 = _mm_div_ps(_mm_set1_ps(1.0f), glmm_vhadd(_mm_mul_ps(x0, r0)));

		Matrix dest;
		glmm_store(dest[0], _mm_mul_ps(v0, x0));
		glmm_store(dest[1], _mm_mul_ps(v1, x0));
		glmm_store(dest[2], _mm_mul_ps(v2, x0));
		glmm_store(dest[3], _mm_mul_ps(v3, x0));
		return dest;
#else
		Matrix tra = Transpose(); //we need to transpose since we're using row-major

		float t[6];
		float det;
		float a = tra[0][0], b = tra[0][1], c = tra[0][2], d = tra[0][3],
			e = tra[1][0], f = tra[1][1], g = tra[1][2], h = tra[1][3],
			i = tra[2][0], j = tra[2][1], k = tra[2][2], l = tra[2][3],
			m = tra[3][0], n = tra[3][1], o = tra[3][2], p = tra[3][3];

		t[0] = k * p - o * l; t[1] = j * p - n * l; t[2] = j * o - n * k;
		t[3] = i * p - m * l; t[4] = i * o - m * k; t[5] = i * n - m * j;

		Matrix dest;
		dest[0][0] = f * t[0] - g * t[1] + h * t[2];
		dest[1][0] = -(e * t[0] - g * t[3] + h * t[4]);
		dest[2][0] = e * t[1] - f * t[3] + h * t[5];
		dest[3][0] = -(e * t[2] - f * t[4] + g * t[5]);

		dest[0][1] = -(b * t[0] - c * t[1] + d * t[2]);
		dest[1][1] = a * t[0] - c * t[3] + d * t[4];
		dest[2][1] = -(a * t[1] - b * t[3] + d * t[5]);
		dest[3][1] = a * t[2] - b * t[4] + c * t[5];

		t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
		t[3] = e * p - m * h; t[4] = e * o - m * g; t[5] = e * n - m * f;

		dest[0][2] = b * t[0] - c * t[1] + d * t[2];
		dest[1][2] = -(a * t[0] - c * t[3] + d * t[4]);
		dest[2][2] = a * t[1] - b * t[3] + d * t[5];
		dest[3][2] = -(a * t[2] - b * t[4] + c * t[5]);

		t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
		t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;

		dest[0][3] = -(b * t[0] - c * t[1] + d * t[2]);
		dest[1][3] = a * t[0] - c * t[3] + d * t[4];
		dest[2][3] = -(a * t[1] - b * t[3] + d * t[5]);
		dest[3][3] = a * t[2] - b * t[4] + c * t[5];

		det = 1.0f / (a * dest[0][0] + b * dest[1][0]
			+ c * dest[2][0] + d * dest[3][0]);

		glm_mat4_scale_p(dest, det);
		return dest;
#endif
	}

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
