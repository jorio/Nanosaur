cmake_minimum_required(VERSION 3.17)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

project(Nanosaur C CXX)

set(NANO_TARGET ${PROJECT_NAME})
set(NANO_ICNS cmake/Nanosaur.icns)

#------------------------------------------------------------------------------
# GLOBAL OPTIONS (BEFORE ADDING SUBDIRECTORIES)
#------------------------------------------------------------------------------

if(MSVC)
    # This lets us detect that MSVC supports C++20 features via the __cplusplus macro
    # (See: https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
    add_compile_options(/Zc:__cplusplus)
    add_compile_definitions(UNICODE _UNICODE)
endif()

#------------------------------------------------------------------------------
# DEPENDENCIES
#------------------------------------------------------------------------------

add_subdirectory(extern/Quesa)
add_subdirectory(extern/Pomme)

find_package(SDL2 REQUIRED COMPONENTS main)
find_package(OpenGL REQUIRED)

#------------------------------------------------------------------------------
# GAME SOURCES
#------------------------------------------------------------------------------

set(NANO_SRC src/game)

set(NANO_SRC_SYSTEM
    ${NANO_SRC}/Main.c
    ${NANO_SRC}/File.c
    ${NANO_SRC}/Collision.c
    ${NANO_SRC}/input.c
    ${NANO_SRC}/Misc.c
    ${NANO_SRC}/Objects.c
    ${NANO_SRC}/Objects2.c
    ${NANO_SRC}/Sprites.c
    ${NANO_SRC}/Sound.c
    ${NANO_SRC}/Windows.c
    ${NANO_SRC}/SelfRunDemo.c
)

set(NANO_SRC_TERRAIN
    ${NANO_SRC}/Terrain.c
    ${NANO_SRC}/Terrain2.c
)

set(NANO_SRC_PLAYER
    ${NANO_SRC}/MyGuy.c
    ${NANO_SRC}/Player_Control.c
    ${NANO_SRC}/Weapons.c
)

set(NANO_SRC_ENEMIES
    ${NANO_SRC}/Enemy.c
    ${NANO_SRC}/Enemy_TriCer.c
    ${NANO_SRC}/Enemy_Stego.c
    ${NANO_SRC}/Enemy_Rex.c
    ${NANO_SRC}/Enemy_Ptera.c
    ${NANO_SRC}/Enemy_Spitter.c
)

set(NANO_SRC_ITEMS
    ${NANO_SRC}/Items.c
    ${NANO_SRC}/TimePortal.c
    ${NANO_SRC}/Pickups.c
    ${NANO_SRC}/Effects.c
    ${NANO_SRC}/Traps.c
    ${NANO_SRC}/Triggers.c
)

set(NANO_SRC_SKELETON
    ${NANO_SRC}/SkeletonObj.c
    ${NANO_SRC}/SkeletonJoints.c
    ${NANO_SRC}/SkeletonAnim.c
    ${NANO_SRC}/Bones.c
)

set(NANO_SRC_QD3D
    ${NANO_SRC}/EnvironmentMap.c
    ${NANO_SRC}/Camera.c
    ${NANO_SRC}/QD3D_Support.c
    ${NANO_SRC}/QD3D_Geometry.c
    ${NANO_SRC}/3DMF.c
    ${NANO_SRC}/3DMath.c
)

set(NANO_SRC_SCREENS
    ${NANO_SRC}/Infobar.c
    ${NANO_SRC}/Title.c
    ${NANO_SRC}/MainMenu.c
    ${NANO_SRC}/HighScores.c
)

set(NANO_HEADERS
    ${NANO_SRC}/main.h
    ${NANO_SRC}/file.h
    ${NANO_SRC}/collision.h
    ${NANO_SRC}/input.h
    ${NANO_SRC}/misc.h
    ${NANO_SRC}/objects.h
    ${NANO_SRC}/3dmath.h
    ${NANO_SRC}/3dmf.h
    ${NANO_SRC}/bones.h
    ${NANO_SRC}/camera.h
    ${NANO_SRC}/effects.h
    ${NANO_SRC}/enemy.h
    ${NANO_SRC}/environmentmap.h
    ${NANO_SRC}/globals.h
    ${NANO_SRC}/highscores.h
    ${NANO_SRC}/infobar.h
    ${NANO_SRC}/items.h
    ${NANO_SRC}/mainmenu.h
    ${NANO_SRC}/mobjtypes.h
    ${NANO_SRC}/movie.h
    ${NANO_SRC}/myguy.h
    ${NANO_SRC}/mytraps.h
    ${NANO_SRC}/objtypes.h
    ${NANO_SRC}/pickups.h
    ${NANO_SRC}/player_control.h
    ${NANO_SRC}/qd3d_geometry.h
    ${NANO_SRC}/qd3d_support.h
    ${NANO_SRC}/selfrundemo.h
    ${NANO_SRC}/skeletonanim.h
    ${NANO_SRC}/skeletonjoints.h
    ${NANO_SRC}/skeletonobj.h
    ${NANO_SRC}/sound2.h
    ${NANO_SRC}/sprites.h
    ${NANO_SRC}/Structs.h
    ${NANO_SRC}/terrain.h
    ${NANO_SRC}/timeportal.h
    ${NANO_SRC}/title.h
    ${NANO_SRC}/triggers.h
    ${NANO_SRC}/weapons.h
    ${NANO_SRC}/windows_nano.h
)

set(SRC_GLUE
    src/nsaur.cpp
    src/Backdrop.cpp
    src/GamePatches.cpp
    src/GamePatches.h
    src/GLBackdrop.cpp
    src/GLBackdrop.h
    src/GLFunctions.cpp
    src/GLFunctions.h
    src/SettingsScreen.cpp
    src/qd3d/QD3D.cpp
    src/qd3d/QD3D.h
    src/qd3d/QD3DCamera.h
    src/qd3d/QD3DDrawContext.h
    src/qd3d/QD3DErrors.h
    src/qd3d/QD3DGeometry.h
    src/qd3d/QD3DGroup.h
    src/qd3d/QD3DIO.h
    src/qd3d/QD3DLight.h
    src/qd3d/QD3DMath.h
    src/qd3d/QD3DPick.h
    src/qd3d/QD3DRenderer.h
    src/qd3d/QD3DSet.h
    src/qd3d/QD3DShader.h
    src/qd3d/QD3DStorage.h
    src/qd3d/QD3DStyle.h
    src/qd3d/QD3DTransform.h
    src/qd3d/QD3DView.h
)

set(NANO_GAME_SOURCES
    ${NANO_HEADERS}
    ${NANO_SRC_SYSTEM}
    ${NANO_SRC_TERRAIN}
    ${NANO_SRC_PLAYER}
    ${NANO_SRC_ENEMIES}
    ${NANO_SRC_ITEMS}
    ${NANO_SRC_SKELETON}
    ${NANO_SRC_QD3D}
    ${NANO_SRC_SCREENS}
)

set(NANO_ALL_SOURCES
    ${NANO_GAME_SOURCES}
    ${SRC_GLUE}
)

#------------------------------------------------------------------------------
# SOURCE GROUPS
#------------------------------------------------------------------------------

source_group(System     FILES ${NANO_SRC_SYSTEM})
source_group(Terrain    FILES ${NANO_SRC_TERRAIN})
source_group(Player     FILES ${NANO_SRC_PLAYER})
source_group(Enemies    FILES ${NANO_SRC_ENEMIES})
source_group(Items      FILES ${NANO_SRC_ITEMS})
source_group(Skeleton   FILES ${NANO_SRC_SKELETON})
source_group(QD3D       FILES ${NANO_SRC_QD3D})
source_group(Screens    FILES ${NANO_SRC_SCREENS})
source_group(Headers    FILES ${NANO_HEADERS})
source_group(Glue       FILES ${SRC_GLUE})

#------------------------------------------------------------------------------
# SOURCE FILE PROPERTIES
#------------------------------------------------------------------------------

if(NOT MSVC)
    set_source_files_properties(${NANO_GAME_SOURCES} PROPERTIES COMPILE_FLAGS "-include Pomme.h")
else()
    set_source_files_properties(${NANO_GAME_SOURCES} PROPERTIES COMPILE_FLAGS /FIPomme.h)
endif()

#------------------------------------------------------------------------------
# EXECUTABLE TARGET
#------------------------------------------------------------------------------

if(WIN32 AND CMAKE_BUILD_TYPE MATCHES Release)
    # "WIN32" here is equivalent to /SUBSYSTEM:WINDOWS for MSVC
    add_executable(${NANO_TARGET} WIN32 ${NANO_ALL_SOURCES})
elseif(APPLE)
    add_executable(${NANO_TARGET} MACOSX_BUNDLE
        ${NANO_ALL_SOURCES}
        ${NANO_ICNS}
        ${SDL2_LIBRARIES}
    )
else()
    add_executable(${NANO_TARGET} ${NANO_ALL_SOURCES})
endif()

target_include_directories(${NANO_TARGET} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    extern/Pomme/src
    src
    src/game
    src/qd3d
)

target_link_libraries(${NANO_TARGET}
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    Quesa_SDL
    Pomme
)

#------------------------------------------------------------------------------
# DEFINES
#------------------------------------------------------------------------------

target_compile_definitions(Quesa_SDL PUBLIC
    QUESA_OS_SDL
    QUESA_APPLY_FOG_COLOR_IN_ALL_PASSES
)

target_compile_definitions(${NANO_TARGET} PRIVATE
    QUESA_OS_SDL
    SOURCE_PORT_ENHANCEMENTS
)

if(WIN32)
    # On Win32, we need NOGDI and NOUSER to be able to define some Mac functions
    # whose names are otherwise taken by Windows APIs.
    target_compile_definitions(${NANO_TARGET} PRIVATE WIN32_LEAN_AND_MEAN NOGDI NOUSER)
endif()

#------------------------------------------------------------------------------
# COMPILER OPTIONS
#------------------------------------------------------------------------------

if(NOT MSVC)
    target_compile_options(${NANO_TARGET} PRIVATE
        -fexceptions
        -Wall
        -Wextra
        -Wshadow
        -Wno-multichar
        -Wno-unknown-pragmas
        -Wno-unused-parameter
    )
else()
    target_compile_options(${NANO_TARGET} PRIVATE
        /EHa    # C++ may throw exceptions from extern "C" functions
        /W4
        /wd4068 # ignore unrecognized pragmas
        /wd4100 # unreferenced formal parameters
        /wd4201 # nonstandard extension (nameless struct)
        /wd4244 # conversion from double to float
        /wd4305 # truncation from double to float
    )
endif()

#------------------------------------------------------------------------------
# PLATFORM-SPECIFIC PACKAGING
#------------------------------------------------------------------------------

if(APPLE)
    # Set macOS target version for all subprojects
    set(MACOS_TARGET_VERSION "10.11")
    set_target_properties(Quesa_SDL       PROPERTIES XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${MACOS_TARGET_VERSION})
    set_target_properties(Pomme           PROPERTIES XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${MACOS_TARGET_VERSION})

    set_target_properties(${NANO_TARGET} PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${MACOS_TARGET_VERSION}

        # Set framework search path to (App bundle)/Contents/Frameworks so the game can use its embedded SDL2.framework
        XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"

        RESOURCE ${NANO_ICNS}
        MACOSX_BUNDLE_ICON_FILE Nanosaur.icns
        MACOSX_BUNDLE_COPYRIGHT "© 1998 Pangea Software, Inc."
    )

    # Copy stuff to app bundle contents
    set_source_files_properties(${NANO_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_source_files_properties(${SDL2_LIBRARIES} PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")

    # Copy assets to app bundle
    add_custom_command(TARGET ${NANO_TARGET} POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)

    #add_custom_command(TARGET ${NANO_TARGET} POST_BUILD COMMAND
    #    plutil -replace NSHighResolutionCapable -bool true $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Info.plist
    #)
else()
    # Copy assets besides executable
    add_custom_command(TARGET ${NANO_TARGET} POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data $<TARGET_FILE_DIR:${NANO_TARGET}>)
endif()

# Copy SDL2 DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET ${NANO_TARGET} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${NANO_TARGET}>/Data)
    endforeach()
endif()
