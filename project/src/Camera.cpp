#include "Camera.h"
#include "SDL_keyboard.h"

using namespace dae;

Camera::Camera( const Vector3& origin, float fovAngle, float aspectRatio, float near, float far )
	: m_Origin{ origin }
	, m_FovAngle{ fovAngle }
	, m_AspectRatio{ aspectRatio }
	, m_Near{ near }
	, m_Far{ far }
{
	SetFovAngleDegrees( fovAngle );
}

// Getters
const Matrix& Camera::GetViewMatrix() const
{
	return m_ViewMatrix;
}

Matrix Camera::GetProjectionMatrix() const
{
	return Matrix::CreatePerspectiveFovLH( m_Fov, m_AspectRatio, m_Near, m_Far );
}

const Vector3& Camera::GetPosition() const
{
	return m_Origin;
}

float Camera::GetFov() const
{
	return m_Fov;
}

float Camera::GetFovAngle() const
{
	return m_FovAngle;
}

float Camera::GetNear() const
{
	return m_Near;
}

float Camera::GetFar() const
{
	return m_Far;
}

// Setters
void Camera::SetPos( const Vector3& newPos )
{
	m_Origin = newPos;
}

void Camera::SetFovAngleDegrees( float newFovAngle )
{
	m_FovAngle = newFovAngle / 180.f * PI;
	m_Fov = tanf( m_FovAngle * 0.5f );
}

// Methods
void Camera::Update( Timer* pTimer )
{
	constexpr float radianConstant{ 1.f / 180.f * PI };
	constexpr float sensitivity{ 0.25f };

	const float deltaTime = pTimer->GetElapsed();
	float speedMultiplier{ 10.f };

	// Keyboard Input
	const uint8_t* pKeyboardState{ SDL_GetKeyboardState( nullptr ) };

	if ( pKeyboardState[SDL_SCANCODE_LSHIFT] )
	{
		speedMultiplier *= 5.f;
	}
	if ( pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP] )
	{
		const Matrix yawRotation{ Matrix::CreateRotationY( m_TotalYaw ) };
		const Matrix pitchRotation{ Matrix::CreateRotationX( m_TotalPitch ) };
		const Matrix rotation{ pitchRotation * yawRotation };

		Move( rotation.TransformVector( Vector3::UnitZ * deltaTime * speedMultiplier ) );
	}
	if ( pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN] )
	{
		const Matrix yawRotation{ Matrix::CreateRotationY( m_TotalYaw ) };
		const Matrix pitchRotation{ Matrix::CreateRotationX( m_TotalPitch ) };
		const Matrix rotation{ pitchRotation * yawRotation };

		Move( rotation.TransformVector( -Vector3::UnitZ * deltaTime * speedMultiplier ) );
	}
	if ( pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT] )
	{
		Move( Matrix::CreateRotationY( m_TotalYaw ).TransformVector( Vector3::UnitX * deltaTime * speedMultiplier ) );
	}
	if ( pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT] )
	{
		Move( Matrix::CreateRotationY( m_TotalYaw ).TransformVector( -Vector3::UnitX * deltaTime * speedMultiplier ) );
	}
	if ( pKeyboardState[SDL_SCANCODE_SPACE] )
	{
		Move( Vector3::UnitY * deltaTime * speedMultiplier );
	}
	if ( pKeyboardState[SDL_SCANCODE_C] )
	{
		Move( -Vector3::UnitY * deltaTime * speedMultiplier );
	}

	// Mouse Input
	int mouseX{}, mouseY{};
	const uint32_t mouseState = SDL_GetRelativeMouseState( &mouseX, &mouseY );

	if ( mouseState == SDL_BUTTON_RMASK )
	{
		Rotate( mouseX * radianConstant * sensitivity, -mouseY * radianConstant * sensitivity );
	}

	if ( mouseState == SDL_BUTTON_LMASK )
	{
		Rotate( mouseX * radianConstant * sensitivity, 0.f );

		const Matrix yawRotation{ Matrix::CreateRotationY( m_TotalYaw ) };
		const Matrix pitchRotation{ Matrix::CreateRotationX( m_TotalPitch ) };
		const Matrix rotation{ pitchRotation * yawRotation };

		Move( rotation.TransformVector( Vector3::UnitZ * -mouseY * sensitivity * 0.5f ) );
	}

	if ( ( mouseState & ( SDL_BUTTON_LMASK | SDL_BUTTON_RMASK ) ) == ( SDL_BUTTON_LMASK | SDL_BUTTON_RMASK ) )
	{
		Move( Matrix::CreateRotationY( m_TotalYaw ).TransformVector( Vector3::UnitY * mouseY * sensitivity ) );
	}

	// Update ONB if needed
	m_ViewMatrix = CalculateViewMatrix();
}

void Camera::Move( const Vector3& change )
{
	m_Origin += change;
}

void Camera::Rotate( float yaw, float pitch )
{
	m_TotalPitch = std::clamp( m_TotalPitch + pitch, -PI * 0.49999f, PI * 0.49999f );
	m_TotalYaw += yaw;

	const Matrix yawRotation{ Matrix::CreateRotationY( m_TotalYaw ) };
	const Matrix pitchRotation{ Matrix::CreateRotationX( m_TotalPitch ) };
	const Matrix rotation{ pitchRotation * yawRotation };

	m_Forward = rotation.TransformVector( Vector3::UnitZ );
}

Matrix Camera::CalculateViewMatrix()
{
	return Matrix::CreateLookAtLH( m_Origin, m_Forward );
}
