#ifndef SAMPLERSTATE_H
#define SAMPLERSTATE_H
#define WIN32_LEAN_AND_MEAN
#include <d3dx11effect.h>

class Sampler
{
public:
	Sampler() = default;
	Sampler( ID3D11Device* pDevice, ID3DX11Effect* pEffect );
	Sampler( const Sampler& ) = delete;
	Sampler& operator=( const Sampler& ) = delete;
	Sampler( Sampler&& rhs );
	Sampler& operator=( Sampler&& rhs );
	~Sampler();

	void Cycle();

	enum class FilterMode
	{
		point,
		linear,
		anisotropic,
		count,
	};

private:
	// SOFTWARE RESOURCES
	FilterMode m_CurrentFilterMode{};
	//

	// HARDWARE RESOURCES: OWNING
	ID3DX11EffectSamplerVariable* m_pSamplerVariable{};
	ID3D11SamplerState* m_pPointSampler{};
	ID3D11SamplerState* m_pLinearSampler{};
	ID3D11SamplerState* m_pAnisotropicSampler{};
	//

	void Update();
	void IncrementFilterMode();
};

#endif
