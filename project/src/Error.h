#ifndef ERROR_H
#define ERROR_H
// This header provides custom error types and functions to simplify exception handling
// Polymorphic -> should be caught by reference
// Uses namespaces to clearly categorize which errors belong to what
#include <cstdint>
#include <iostream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace error
{
class Error
{
public:
	virtual std::string category() const
	{
		return "ERR";
	}
	virtual std::string what() const
	{
		return "Error";
	}
};

namespace file
{
class FileError : public Error
{
public:
	virtual std::string category() const
	{
		return "FILE_ERR";
	}
};

class CouldNotOpenFile : public FileError
{
public:
	virtual std::string what() const
	{
		return "CouldNotOpenFile";
	}
};
} // namespace file

namespace effect
{
class EffectError : public Error
{
public:
	virtual std::string category() const
	{
		return "FX_ERR";
	}
};

class CreateFail : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "CreateFail";
	}
};

class InvalidEffect : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidEffect";
	}
};

class LayoutCreateFail : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "LayoutCreateFail";
	}
};

class InvalidTechnique : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidTechnique";
	}
};

class InvalidWorldViewProjection : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidWorldViewProjection";
	}
};

class InvalidWorld : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidWorld";
	}
};

class InvalidCameraOrigin : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidCameraOrigin";
	}
};

class InvalidMap : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidDiffuseMap";
	}
};

class InvalidSampler : public EffectError
{
public:
	virtual std::string what() const override
	{
		return "InvalidSampler";
	}
};
} // namespace effect

namespace texture
{
class TextureError : public Error
{
public:
	virtual std::string category() const override
	{
		return "TEX_ERR";
	}
};

class ResourceCreateFail : public TextureError
{
public:
	virtual std::string what() const override
	{
		return "ResourceCreateFail";
	}
};

class ResourceViewCreateFail : public TextureError
{
public:
	virtual std::string what() const override
	{
		return "ResourceViewCreateFail";
	}
};
} // namespace texture

namespace mesh
{
class MeshError : public Error
{
public:
	virtual std::string category() const override
	{
		return "MESH_ERR";
	}
};

class CreateFail : public MeshError
{
public:
	virtual std::string what() const override
	{
		return "CreateFail";
	}
};

class BufferCreateFail : public MeshError
{
public:
	virtual std::string what() const override
	{
		return "BufferCreateFail";
	}
};

class BufferIsEmpty : public MeshError
{
public:
	virtual std::string what() const override
	{
		return "BufferIsEmpty";
	}
};

class WrongTopology : public MeshError
{
public:
	virtual std::string what() const override
	{
		return "WrongTopology";
	}
};
} // namespace mesh

namespace scene
{
class SceneError : public Error
{
public:
	virtual std::string category() const override
	{
		return "SCENE_ERR";
	}
};

class SceneIsEmpty : public SceneError
{
public:
	virtual std::string what() const override
	{
		return "SceneIsEmpty";
	}
};
} // namespace scene

namespace rendering
{
class RenderError : public Error
{
public:
	virtual std::string category() const override
	{
		return "RENDER_ERR";
	}
};

class MeshRenderError : public RenderError
{
public:
	virtual std::string what() const override
	{
		return "MeshRenderError";
	}
};
} // namespace rendering

namespace dx11
{
class DXInitError : public Error
{
public:
	virtual std::string category() const override
	{
		return "DX_INIT_ERR";
	}
};

class DeviceCreateFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "DeviceCreateFail";
	}
};

class DXGIFactoryCreateFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "DXGIFactoryCreateFail";
	}
};

class SwapChainCreateFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "SwapChainCreateFail";
	}
};

class DepthStencilCreateFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "DepthStencilCreateFail";
	}
};

class DepthStencilViewCreateFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "DepthStencilViewCreateFail";
	}
};

class GetRenderTargetBufferFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "GetRenderTargetBufferFail";
	}
};

class RenderTargetViewCreateFail : public DXInitError
{
public:
	virtual std::string what() const override
	{
		return "RenderTargetViewCreateFail";
	}
};
} // namespace dx11

namespace utils
{
template <typename Function>
bool HandleThrowingFunction( Function f ) noexcept // All exceptions are contained within this function -> doesn't throw
{
	try
	{
		f();
	}
	catch ( const Error& e )
	{
		HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
		SetConsoleTextAttribute( consoleHandle, 12 );

		std::cout << "[" << e.category() << "]: " << e.what() << "\n";

		SetConsoleTextAttribute( consoleHandle, 7 );
		return true;
	}
	catch ( const std::exception& e )
	{
		HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
		SetConsoleTextAttribute( consoleHandle, 12 );

		std::cout << "Caught exception: " << e.what() << "\n";

		SetConsoleTextAttribute( consoleHandle, 7 );
		return true;
	}
	catch ( const std::string& eString )
	{
		HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
		SetConsoleTextAttribute( consoleHandle, 12 );

		std::cout << "Caught exception: " << eString << "\n";

		SetConsoleTextAttribute( consoleHandle, 7 );
		return true;
	}
	catch ( int eCode )
	{
		HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
		SetConsoleTextAttribute( consoleHandle, 12 );

		std::cout << "Caught exception: CODE=[0x" << std::hex << eCode << "]\n";

		SetConsoleTextAttribute( consoleHandle, 7 );
		return true;
	}
	catch ( uint32_t eCode )
	{
		HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
		SetConsoleTextAttribute( consoleHandle, 12 );

		std::cout << "Caught exception: CODE=[0x" << std::hex << eCode << "]\n";

		SetConsoleTextAttribute( consoleHandle, 7 );
		return true;
	}
	catch ( ... )
	{
		HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
		SetConsoleTextAttribute( consoleHandle, 12 );

		std::cout << "Caught unhandled exception\n";

		SetConsoleTextAttribute( consoleHandle, 7 );
		return true;
	}

	return false;
}
} // namespace utils
} // namespace error
#endif
