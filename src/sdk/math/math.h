#pragma once

namespace math
{
	struct vector2 final
	{
		float x{ 0.f }, y{ 0.f };
	};

	struct vector3 final
	{
		float x{ 0.f }, y{ 0.f }, z{ 0.f };
	};

	inline vector3 operator+(const vector3& a, const vector3& b)
	{
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}

	struct vector4 final
	{
		float x{ 0.f }, y{ 0.f }, z{ 0.f }, w{ 0.f };
	};

	struct matrix3 final
	{
		float m[9];
	};

	inline vector3 operator*(const matrix3& m, const vector3& v)
	{
		return
		{
			m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z,
			m.m[3] * v.x + m.m[4] * v.y + m.m[5] * v.z,
			m.m[6] * v.x + m.m[7] * v.y + m.m[8] * v.z
		};
	}

	struct matrix4 final
	{
		float m[4][4];

		vector4 multiply(const vector4& v) const
		{
			return {
				m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
				m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
				m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
				m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
			};
		}
	};
}