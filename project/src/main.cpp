// External includes
#include <SDL.h>
#include <SDL_surface.h>
#include <SDL_image.h>
#include <SDL_syswm.h>
#include <SDL_video.h>
#include <consoleapi2.h>
#include "Error.h"

// Windows header
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// Than you microsoft
#undef main
//

// Standard includes
#include <iostream>
#include <memory>

// Project includes
#include "Timer.h"
#include "HardwareRenderer.h"
#include "SoftwareRenderer.h"
#if defined( _DEBUG )
#	include "LeakDetector.h"
#endif

using namespace dae;

void ShutDown( SDL_Window* pWindow )
{
	SDL_DestroyWindow( pWindow );
	SDL_Quit();
}

void SetCoutColor( int color )
{
	HANDLE consoleHandle{ GetStdHandle( STD_OUTPUT_HANDLE ) };
	SetConsoleTextAttribute( consoleHandle, color );
}

int main( int argc, char* args[] )
{
	// Unreferenced parameters
	(void)argc;
	(void)args;

// Leak detection
#if defined( _DEBUG )
	LeakDetector detector{};
#endif

	// Create window + surfaces
	SDL_Init( SDL_INIT_VIDEO );

	const uint32_t width = 640;
	const uint32_t height = 480;

	const SDL_WindowFlags windowFlags{};

	SDL_Window* pWindow = SDL_CreateWindow( "DirectX - ***Insert Name/Class***",
											SDL_WINDOWPOS_UNDEFINED,
											SDL_WINDOWPOS_UNDEFINED,
											width,
											height,
											windowFlags );

	if ( !pWindow )
		return 1;

	// Initialize "framework"
	Timer timer{};
	HardwareRenderer hardwareRenderer{ pWindow };
	SoftwareRenderer softwareRenderer{ pWindow };

	// Initialize scene
	std::vector<std::unique_ptr<Scene>> scenePtrs{};
	scenePtrs.push_back( std::make_unique<VehicleScene>() );
	error::utils::HandleThrowingFunction( [&]() {
		for ( auto& pScene : scenePtrs )
		{
			hardwareRenderer.InitScene( pScene.get() );
		}
	} );
	// TODO:Add scene switching
	size_t sceneIdx{ 0 };

	// Start loop
	timer.Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while ( isLooping )
	{
		//--------- Get input events ---------
		SDL_Event e{};
		while ( SDL_PollEvent( &e ) )
		{
			switch ( e.type )
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				// Test for a key
				// if (e.key.keysym.scancode == SDL_SCANCODE_X)
				break;
			default:;
			}
		}

		//--------- Update ---------
		// hardwareRenderer.Update( timer );
		softwareRenderer.Update( timer );
		scenePtrs[sceneIdx]->Update( &timer );

		//--------- Render ---------
		// hardwareRenderer.Render( scenePtrs[sceneIdx].get() );
		softwareRenderer.Render( scenePtrs[sceneIdx].get() );

		//--------- Timer ----------
		timer.Update();
		printTimer += timer.GetElapsed();
		if ( printTimer >= 1.f )
		{
			printTimer = 0.f;
			std::cout << "dFPS: " << timer.GetdFPS() << std::endl;
		}
	}
	timer.Stop();

	ShutDown( pWindow );
}
