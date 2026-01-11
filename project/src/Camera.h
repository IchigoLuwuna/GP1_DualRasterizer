#ifndef CAMERA_H
#define CAMERA_H
#include <SDL_mouse.h>
#include "Timer.h"
#include "Matrix.h"

namespace dae
{
class Camera final
{
public:
	Camera() = default;
	explicit Camera( const Vector3& origin,
					 float fovAngle,
					 float aspectRatio = ( 4.f / 3.f ),
					 float near = 0.1f,
					 float far = 100.f );

	// Getters
	const Matrix& GetViewMatrix() const;
	Matrix GetProjectionMatrix() const;
	const Vector3& GetPosition() const;
	float GetFov() const;
	float GetFovAngle() const;
	float GetNear() const;
	float GetFar() const;

	// Setters
	void SetPos( const Vector3& newPos );
	void SetFovAngleDegrees( float newFovAngle );

	// Methods
	void Update( Timer* pTimer );
	void Move( const Vector3& change );
	void Rotate( float yaw, float pitch );

private:
	// Members
	Vector3 m_Origin{};
	Vector3 m_Forward{ Vector3::UnitZ };

	float m_FovAngle{ 60.f / 180.f * PI };
	float m_Fov{ tanf( m_FovAngle * 0.5f ) };
	float m_TotalPitch{ 0.f };
	float m_TotalYaw{ 0.f };

	float m_AspectRatio{};

	float m_Near{};
	float m_Far{};

	Matrix m_ViewMatrix{};

	// Methods
	Matrix CalculateViewMatrix();
};
} // namespace dae
#endif
