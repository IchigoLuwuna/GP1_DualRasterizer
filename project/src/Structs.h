#ifndef STRUCTS_H
#define STRUCTS_H
#include <array>
#include "ColorRGB.h"

namespace dae
{
struct Vector2;
struct Vector3;
struct Vector4;
struct Vertex;

struct Vector2 final
{
	float x{};
	float y{};

	Vector2() = default;
	Vector2( float _x, float _y );
	Vector2( const Vector2& from, const Vector2& to );

	float Magnitude() const;
	float SqrMagnitude() const;
	float Normalize();
	Vector2 Normalized() const;

	static float Dot( const Vector2& v1, const Vector2& v2 );
	static float Cross( const Vector2& v1, const Vector2& v2 );

	// Member Operators
	Vector2 operator*( float scale ) const;
	Vector2 operator/( float scale ) const;
	Vector2 operator+( const Vector2& v ) const;
	Vector2 operator-( const Vector2& v ) const;
	Vector2 operator-() const;
	Vector2& operator+=( const Vector2& v );
	Vector2& operator-=( const Vector2& v );
	Vector2& operator/=( float scale );
	Vector2& operator*=( float scale );
	float& operator[]( int index );
	float operator[]( int index ) const;

	static const Vector2 UnitX;
	static const Vector2 UnitY;
	static const Vector2 Zero;
};

struct Vector3 final
{
	float x{};
	float y{};
	float z{};

	Vector3() = default;
	Vector3( float _x, float _y, float _z );
	Vector3( const Vector3& from, const Vector3& to );
	Vector3( const Vector4& v );

	float Magnitude() const;
	float SqrMagnitude() const;
	float Normalize();
	Vector3 Normalized() const;

	static float Dot( const Vector3& v1, const Vector3& v2 );
	static Vector3 Cross( const Vector3& v1, const Vector3& v2 );
	static Vector3 Project( const Vector3& v1, const Vector3& v2 );
	static Vector3 Reject( const Vector3& v1, const Vector3& v2 );
	static Vector3 Reflect( const Vector3& v1, const Vector3& v2 );

	Vector4 ToPoint4() const;
	Vector4 ToVector4() const;
	Vector2 GetXY() const;

	// Member Operators
	Vector3 operator*( float scale ) const;
	Vector3 operator/( float scale ) const;
	Vector3 operator+( const Vector3& v ) const;
	Vector3 operator-( const Vector3& v ) const;
	Vector3 operator-() const;
	Vector3& operator+=( const Vector3& v );
	Vector3& operator-=( const Vector3& v );
	Vector3& operator/=( float scale );
	Vector3& operator*=( float scale );
	float& operator[]( int index );
	float operator[]( int index ) const;
	bool operator==( const Vector3& v ) const;

	static const Vector3 UnitX;
	static const Vector3 UnitY;
	static const Vector3 UnitZ;
	static const Vector3 Zero;
};

struct Vector4 final
{
	float x;
	float y;
	float z;
	float w;

	Vector4() = default;
	Vector4( float _x, float _y, float _z, float _w );
	Vector4( const Vector3& v, float _w );

	float Magnitude() const;
	float SqrMagnitude() const;
	float Normalize();
	Vector4 Normalized() const;

	Vector2 GetXY() const;
	Vector3 GetXYZ() const;

	static float Dot( const Vector4& v1, const Vector4& v2 );

	// operator overloading
	Vector4 operator*( float scale ) const;
	Vector4 operator+( const Vector4& v ) const;
	Vector4 operator-( const Vector4& v ) const;
	Vector4& operator+=( const Vector4& v );
	float& operator[]( int index );
	float operator[]( int index ) const;
	bool operator==( const Vector4& v ) const;
};

struct Vertex final
{
	Vector3 position{};
	ColorRGB color{};
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
};

struct VertexOut final
{
	Vector4 position{};
	Vector3 worldPosition{};
	ColorRGB color{};
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
};

struct Rectangle
{
	float left{};
	float right{};
	float top{};
	float bottom{};
};

struct TriangleWorld
{
	TriangleWorld() = default;
	TriangleWorld( const Vertex& v0, const Vertex& v1, const Vertex& v2 )
		: v0{ v0 }
		, v1{ v1 }
		, v2{ v2 }
		, normal{
			Vector3::Cross( Vector3( v0.position, v2.position ), Vector3( v0.position, v1.position ) ).Normalized()
		}
	{
	}

	Vertex v0{};
	Vertex v1{};
	Vertex v2{};

	Vector3 normal{};
};

struct TriangleOut
{
	TriangleOut() = default;
	TriangleOut( const VertexOut& v0, const VertexOut& v1, const VertexOut& v2 )
		: v0{ v0 }
		, v1{ v1 }
		, v2{ v2 }
		, normal{ Vector3::Cross( Vector3( { v0.position.x, v0.position.y, v0.position.w },
										   { v2.position.x, v2.position.y, v2.position.w } ),
								  Vector3( { v0.position.x, v0.position.y, v0.position.w },
										   { v1.position.x, v1.position.y, v1.position.w } ) )
					  .Normalized() }
	{
	}

	VertexOut v0{};
	VertexOut v1{};
	VertexOut v2{};

	Vector3 normal{};

	Rectangle GetBounds() const
	{
		Rectangle rectangle{};
		std::array<float, 3> axisContainer{};

		axisContainer[0] = v0.position.x;
		axisContainer[1] = v1.position.x;
		axisContainer[2] = v2.position.x;

		rectangle.left = *std::min_element( axisContainer.begin(), axisContainer.end() );
		rectangle.right = *std::max_element( axisContainer.begin(), axisContainer.end() );

		axisContainer[0] = v0.position.y;
		axisContainer[1] = v1.position.y;
		axisContainer[2] = v2.position.y;

		rectangle.top = *std::min_element( axisContainer.begin(), axisContainer.end() );
		rectangle.bottom = *std::max_element( axisContainer.begin(), axisContainer.end() );

		return rectangle;
	}
};

// Global Operators
inline Vector2 operator*( float scale, const Vector2& v )
{
	return { v.x * scale, v.y * scale };
}
inline Vector3 operator*( float scale, const Vector3& v )
{
	return { v.x * scale, v.y * scale, v.z * scale };
}
} // namespace dae
#endif
