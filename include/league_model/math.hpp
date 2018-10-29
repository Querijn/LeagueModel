#pragma once

namespace Math
{
	struct Float2
	{
		Float2() {}

		Float2(float a_X, float a_Y) :
			x(a_X), y(a_Y)		
		{
		}

		Float2(const float* a_Float2) :
			x(a_Float2[0]), y(a_Float2[1])
		{
		}

		union
		{
			float Data[2];
			struct
			{
				float x;
				float y;
			};
			struct
			{
				float u;
				float v;
			};
		};
	};

	struct Float3
	{
		Float3() {}

		Float3(float a_X, float a_Y, float a_Z) :
			x(a_X), y(a_Y), z(a_Z)
		{
		}

		Float3(const float* a_Float3) :
			x(a_Float3[0]), y(a_Float3[1]), z(a_Float3[2])
		{
		}

		union
		{
			float Data[3];
			struct
			{
				float x;
				float y;
				float z;
			};
		};
	};

	struct Float4
	{
		Float4() {}

		Float4(float a_X, float a_Y, float a_Z, float a_W) :
			x(a_X), y(a_Y), z(a_Z), w(a_W)
		{
		}

		Float4(const float* a_Float4) :
			x(a_Float4[0]), y(a_Float4[1]), z(a_Float4[2]), w(a_Float4[3])
		{
		}

		union
		{
			float Data[4];
			struct
			{
				float x;
				float y;
				float z;
				float w;
			};
		};
	};
}