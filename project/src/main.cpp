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
#include "Renderer.h"
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

void DisplayHelp()
{
	std::cout << "[F1]: Toggle Hardware/Software Rendering\n"
			  << "[F2]: Toggle Vehicle Rotation\n"
			  << "[F10]: Toggle Uniform Clear Color\n"
			  << "[F11]: Toggle FPS\n"
			  << "[F12]: Show Help (This)\n\n"

			  << "[F3]: Toggle Fire Effect (Hardware Only)\n"
			  << "[F4]: Cycle Sampling Method (Hardware Only)\n\n"

			  << "[F5]: Cycle Shading Mode(Software Only)\n"
			  << "[F6]: Toggle Normal Map(Software Only)\n"
			  << "[F7]: Toggle Depth Buffer Visualization (Software Only)\n"
			  << "[F8]: Toggle Bounding Box Visualization (Software Only)\n";
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
	Renderer renderer{ pWindow };

	// Initialize scene
	std::vector<std::unique_ptr<Scene>> scenePtrs{}; // allows for multiple scenes in a project
	scenePtrs.push_back( std::make_unique<VehicleScene>() );
	error::utils::HandleThrowingFunction( [&]() {
		for ( auto& pScene : scenePtrs )
		{
			renderer.InitScene( pScene.get() );
		}
	} );
	// TODO:Add scene switching
	size_t sceneIdx{ 0 };

	bool displayFPS{ true };

	// Start loop
	timer.Start();
	float printTimer = 0.f;
	bool isLooping = true;
	DisplayHelp();
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
				renderer.HandleKeyUp( e.key );
				scenePtrs[sceneIdx]->HandleKeyUp( e.key );
				if ( e.key.keysym.scancode == SDL_SCANCODE_F11 )
				{
					displayFPS = !displayFPS;
					if ( displayFPS )
					{
						std::cout << "Displaying FPS\n";
					}
					else
					{
						std::cout << "Not displaying FPS\n";
					}
				}

				if ( e.key.keysym.scancode == SDL_SCANCODE_F12 )
				{
					DisplayHelp();
				}
				break;
			default:;
			}
		}

		//--------- Update ---------
		// hardwareRenderer.Update( timer );
		scenePtrs[sceneIdx]->Update( &timer );

		//--------- Render ---------
		renderer.Render( scenePtrs[sceneIdx].get() );

		//--------- Timer ----------
		timer.Update();
		printTimer += timer.GetElapsed();
		if ( printTimer >= 1.f && displayFPS )
		{
			printTimer = 0.f;
			std::cout << "dFPS: " << timer.GetdFPS() << std::endl;
		}
	}
	timer.Stop();

	ShutDown( pWindow );
}
