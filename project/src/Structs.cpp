#include <cassert>
#include <cmath>
#include "MathHelpers.h"
#include "Structs.h"

namespace dae
{
const Vector2 Vector2::UnitX = Vector2{ 1, 0 };
const Vector2 Vector2::UnitY = Vector2{ 0, 1 };
const Vector2 Vector2::Zero = Vector2{ 0, 0 };

Vector2::Vector2( float _x, float _y )
	: x( _x )
	, y( _y )
{
}

Vector2::Vector2( const Vector2& from, const Vector2& to )
	: x( to.x - from.x )
	, y( to.y - from.y )
{
}

float Vector2::Magnitude() const
{
	return std::sqrt( x * x + y * y );
}

float Vector2::SqrMagnitude() const
{
	return x * x + y * y;
}

float Vector2::Normalize()
{
	const float m = Magnitude();
	x /= m;
	y /= m;

	return m;
}

Vector2 Vector2::Normalized() const
{
	const float m = Magnitude();
	return { x / m, y / m };
}

float Vector2::Dot( const Vector2& v1, const Vector2& v2 )
{
	return v1.x * v2.x + v1.y * v2.y;
}

float Vector2::Cross( const Vector2& v1, const Vector2& v2 )
{
	return v1.x * v2.y - v1.y * v2.x;
}

#pragma region Operator Overloads
Vector2 Vector2::operator*( float scale ) const
{
	return { x * scale, y * scale };
}

Vector2 Vector2::operator/( float scale ) const
{
	return { x / scale, y / scale };
}

Vector2 Vector2::operator+( const Vector2& v ) const
{
	return { x + v.x, y + v.y };
}

Vector2 Vector2::operator-( const Vector2& v ) const
{
	return { x - v.x, y - v.y };
}

Vector2 Vector2::operator-() const
{
	return { -x, -y };
}

Vector2& Vector2::operator*=( float scale )
{
	x *= scale;
	y *= scale;
	return *this;
}

Vector2& Vector2::operator/=( float scale )
{
	x /= scale;
	y /= scale;
	return *this;
}

Vector2& Vector2::operator-=( const Vector2& v )
{
	x -= v.x;
	y -= v.y;
	return *this;
}

Vector2& Vector2::operator+=( const Vector2& v )
{
	x += v.x;
	y += v.y;
	return *this;
}

float& Vector2::operator[]( int index )
{
	assert( index <= 1 && index >= 0 );
	return index == 0 ? x : y;
}

float Vector2::operator[]( int index ) const
{
	assert( index <= 1 && index >= 0 );
	return index == 0 ? x : y;
}
#pragma endregion

const Vector3 Vector3::UnitX = Vector3{ 1, 0, 0 };
const Vector3 Vector3::UnitY = Vector3{ 0, 1, 0 };
const Vector3 Vector3::UnitZ = Vector3{ 0, 0, 1 };
const Vector3 Vector3::Zero = Vector3{ 0, 0, 0 };

Vector3::Vector3( float _x, float _y, float _z )
	: x( _x )
	, y( _y )
	, z( _z )
{
}

Vector3::Vector3( const Vector4& v )
	: x( v.x )
	, y( v.y )
	, z( v.z )
{
}

Vector3::Vector3( const Vector3& from, const Vector3& to )
	: x( to.x - from.x )
	, y( to.y - from.y )
	, z( to.z - from.z )
{
}

float Vector3::Magnitude() const
{
	return std::sqrt( x * x + y * y + z * z );
}

float Vector3::SqrMagnitude() const
{
	return x * x + y * y + z * z;
}

float Vector3::Normalize()
{
	const float m = Magnitude();
	x /= m;
	y /= m;
	z /= m;

	return m;
}

Vector3 Vector3::Normalized() const
{
	const float m = Magnitude();
	return { x / m, y / m, z / m };
}

float Vector3::Dot( const Vector3& v1, const Vector3& v2 )
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vector3 Vector3::Cross( const Vector3& v1, const Vector3& v2 )
{
	return Vector3{ v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x };
}

Vector3 Vector3::Project( const Vector3& v1, const Vector3& v2 )
{
	return ( v2 * ( Dot( v1, v2 ) / Dot( v2, v2 ) ) );
}

Vector3 Vector3::Reject( const Vector3& v1, const Vector3& v2 )
{
	return ( v1 - v2 * ( Dot( v1, v2 ) / Dot( v2, v2 ) ) );
}

Vector3 Vector3::Reflect( const Vector3& v1, const Vector3& v2 )
{
	return v1 - ( 2.f * Vector3::Dot( v1, v2 ) * v2 );
}

Vector4 Vector3::ToPoint4() const
{
	return { x, y, z, 1 };
}

Vector4 Vector3::ToVector4() const
{
	return { x, y, z, 0 };
}

Vector2 Vector3::GetXY() const
{
	return { x, y };
}

#pragma region Operator Overloads
Vector3 Vector3::operator*( float scale ) const
{
	return { x * scale, y * scale, z * scale };
}

Vector3 Vector3::operator/( float scale ) const
{
	return { x / scale, y / scale, z / scale };
}

Vector3 Vector3::operator+( const Vector3& v ) const
{
	return { x + v.x, y + v.y, z + v.z };
}

Vector3 Vector3::operator-( const Vector3& v ) const
{
	return { x - v.x, y - v.y, z - v.z };
}

Vector3 Vector3::operator-() const
{
	return { -x, -y, -z };
}

Vector3& Vector3::operator*=( float scale )
{
	x *= scale;
	y *= scale;
	z *= scale;
	return *this;
}

Vector3& Vector3::operator/=( float scale )
{
	x /= scale;
	y /= scale;
	z /= scale;
	return *this;
}

Vector3& Vector3::operator-=( const Vector3& v )
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

Vector3& Vector3::operator+=( const Vector3& v )
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

float& Vector3::operator[]( int index )
{
	assert( index <= 2 && index >= 0 );

	if ( index == 0 )
		return x;
	if ( index == 1 )
		return y;
	return z;
}

float Vector3::operator[]( int index ) const
{
	assert( index <= 2 && index >= 0 );

	if ( index == 0 )
		return x;
	if ( index == 1 )
		return y;
	return z;
}

bool Vector3::operator==( const Vector3& v ) const
{
	return AreEqual( x, v.x ) && AreEqual( y, v.y ) && AreEqual( z, v.z );
}

Vector4::Vector4( float _x, float _y, float _z, float _w )
	: x( _x )
	, y( _y )
	, z( _z )
	, w( _w )
{
}
Vector4::Vector4( const Vector3& v, float _w )
	: x( v.x )
	, y( v.y )
	, z( v.z )
	, w( _w )
{
}

float Vector4::Magnitude() const
{
	return std::sqrt( x * x + y * y + z * z + w * w );
}

float Vector4::SqrMagnitude() const
{
	return x * x + y * y + z * z + w * w;
}

float Vector4::Normalize()
{
	const float m = Magnitude();
	x /= m;
	y /= m;
	z /= m;
	w /= m;

	return m;
}

Vector4 Vector4::Normalized() const
{
	const float m = Magnitude();
	return { x / m, y / m, z / m, w / m };
}

Vector2 Vector4::GetXY() const
{
	return { x, y };
}

Vector3 Vector4::GetXYZ() const
{
	return { x, y, z };
}

float Vector4::Dot( const Vector4& v1, const Vector4& v2 )
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

#pragma region Operator Overloads
Vector4 Vector4::operator*( float scale ) const
{
	return { x * scale, y * scale, z * scale, w * scale };
}

Vector4 Vector4::operator+( const Vector4& v ) const
{
	return { x + v.x, y + v.y, z + v.z, w + v.w };
}

Vector4 Vector4::operator-( const Vector4& v ) const
{
	return { x - v.x, y - v.y, z - v.z, w - v.w };
}

Vector4& Vector4::operator+=( const Vector4& v )
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return *this;
}

float& Vector4::operator[]( int index )
{
	assert( index <= 3 && index >= 0 );

	if ( index == 0 )
		return x;
	if ( index == 1 )
		return y;
	if ( index == 2 )
		return z;
	return w;
}

float Vector4::operator[]( int index ) const
{
	assert( index <= 3 && index >= 0 );

	if ( index == 0 )
		return x;
	if ( index == 1 )
		return y;
	if ( index == 2 )
		return z;
	return w;
}

bool Vector4::operator==( const Vector4& v ) const
{
	return AreEqual( x, v.x, .000001f ) && AreEqual( y, v.y, .000001f ) && AreEqual( z, v.z, .000001f ) &&
		   AreEqual( w, v.w, .000001f );
}
#pragma endregion
} // namespace dae
