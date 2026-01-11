# Source files
set(SOURCES
    "src/main.cpp"
    "src/LeakDetector.cpp"
    "src/Matrix.cpp"
    "src/ColorRGB.cpp"
    "src/Structs.cpp"
    "src/Timer.cpp"
    "src/Renderer.cpp"
    "src/Shading.cpp"
    "src/Effect.cpp"
    "src/Mesh.cpp"
    "src/Scene.cpp"
    "src/Camera.cpp"
    "src/Texture.cpp"
    "src/Sampler.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# DirectX11
option(DIRECTX_11_ENABLED "Enable DirectX 11 Support" ON)
if(DIRECTX_11_ENABLED)
    if(LINUX)
        include(ExternalProject)

        ExternalProject_Add(dxvk-native
        GIT_REPOSITORY https://github.com/doitsujin/dxvk.git
        GIT_TAG v2.3
        CONFIGURE_COMMAND meson setup <SOURCE_DIR>
        BUILD_COMMAND ninja src/d3d11/libdxvk_d3d11.so src/dxgi/libdxvk_dxgi.so
        INSTALL_COMMAND "")

        ExternalProject_Get_property(dxvk-native SOURCE_DIR BINARY_DIR)
        set(DXVK_SOURCE_DIR ${SOURCE_DIR})
        set(DXVK_BINARY_DIR ${BINARY_DIR})
        unset(SOURCE_DIR)
        unset(BINARY_DIR)
        add_dependencies(${PROJECT_NAME} dxvk-native)

        include_directories(SYSTEM
        ${DXVK_SOURCE_DIR}/include/native/directx
        ${DXVK_SOURCE_DIR}/include/native/windows)
        target_link_directories(${PROJECT_NAME} PRIVATE
        ${DXVK_BINARY_DIR}/src/d3d11
        ${DXVK_BINARY_DIR}/src/dxgi
    )

        target_link_libraries(
            ${PROJECT_NAME} PUBLIC
            dxvk_d3d11
            dxvk_dxgi
            SDL2::SDL2
            SDL2_image
        )

        #set(FX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/dx11effects")
        #if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        #set(FX_LIBRARY "${FX_DIR}/lib/x64/dx11effects_d.lib")
        #else()
        #set(FX_LIBRARY "${FX_DIR}/lib/x64/dx11effects.lib")
        #endif()
        #add_library(FX STATIC IMPORTED)
        #set_target_properties(FX PROPERTIES
        #IMPORTED_LOCATION "${FX_LIBRARY}"
        #INTERFACE_INCLUDE_DIRECTORIES "${FX_DIR}/include"
        #)
        #target_link_libraries(${PROJECT_NAME} PRIVATE FX)
    else()
        find_library(DXGI_LIBRARY dxgi.lib)
        find_library(D3D11_LIBRARY d3d11.lib)
        if(DXGI_LIBRARY AND D3D11_LIBRARY)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${DXGI_LIBRARY} ${D3D11_LIBRARY})
        else()
            # Hacky solution to access the DX11 libraries using an absolute path
            # Shouldn't be an issue if you have the DX SDK installed on a windows machine
            #
            if(NOT DXGI_LIBRARY)
                message("DXGI not found, attempting to access directly")
                set(DXGI_LIBRARY "/home/luna/.wine/drive_c/Program\ Files\ \(x86\)/Microsoft\ DirectX\ SDK\ \(June\ 2010\)/Lib/x64/dxgi.lib")
            endif()
            if(NOT D3D11_LIBRARY)
                message("D3D11 not found, attempting to access directly")
                set(D3D11_LIBRARY "/home/luna/.wine/drive_c/Program\ Files\ (x86)/Microsoft\ DirectX\ SDK\ (June\ 2010)/Lib/x64/d3d11.lib")
            endif()

            #Try again :3
            target_link_libraries(${PROJECT_NAME} PRIVATE ${DXGI_LIBRARY} ${D3D11_LIBRARY})
        endif()
    endif()
endif()

# Copy resources to output folder
set(RESOURCES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
file(GLOB_RECURSE RESOURCE_FILES
    "${RESOURCES_SOURCE_DIR}/*.jpg"
    "${RESOURCES_SOURCE_DIR}/*.png"
    "${RESOURCES_SOURCE_DIR}/*.obj"
    "${RESOURCES_SOURCE_DIR}/*.fx"
)
set(RESOURCES_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources/")
file(MAKE_DIRECTORY ${RESOURCES_OUT_DIR})
foreach(RESOURCE ${RESOURCE_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE}
    ${RESOURCES_OUT_DIR})
endforeach()

if(LINUX)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2_image)
else()
    # Simple Directmedia Layer
    set(SDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2-2.30.7")
    add_library(SDL STATIC IMPORTED)
    set_target_properties(SDL PROPERTIES
    IMPORTED_LOCATION "${SDL_DIR}/lib/x64/SDL2.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL_DIR}/include"
)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL)

    file(GLOB_RECURSE DLL_FILES
    "${SDL_DIR}/lib/x64/*.dll"
    "${SDL_DIR}/lib/x64/*.manifest"
)

    foreach(DLL ${DLL_FILES})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endforeach(DLL)

    # Simple Directmedia Layer Image
    set(SDL_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2_image-2.8.2")
    add_library(SDL_IMAGE STATIC IMPORTED)
    set_target_properties(SDL_IMAGE PROPERTIES
    IMPORTED_LOCATION "${SDL_IMAGE_DIR}/lib/x64/SDL2_image.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL_IMAGE_DIR}/include"
)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL_IMAGE)

    file(GLOB_RECURSE DLL_FILES
    "${SDL_IMAGE_DIR}/lib/x64/*.dll"
    "${SDL_IMAGE_DIR}/lib/x64/*.manifest"
)

    foreach(DLL ${DLL_FILES})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endforeach(DLL)

    # DirectX Effects
    if(DIRECTX_11_ENABLED)
        set(FX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/dx11effects")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(FX_LIBRARY "${FX_DIR}/lib/x64/dx11effects_d.lib")
        else()
            set(FX_LIBRARY "${FX_DIR}/lib/x64/dx11effects.lib")
        endif()
        add_library(FX STATIC IMPORTED)
        set_target_properties(FX PROPERTIES
        IMPORTED_LOCATION "${FX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FX_DIR}/include"
    )
        target_link_libraries(${PROJECT_NAME} PRIVATE FX)
    endif()
endif()
