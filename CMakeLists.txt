cmake_minimum_required(VERSION 3.17)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

project(nsaur C CXX)

add_subdirectory(Quesa)

add_subdirectory(src/pomme)

# "WIN32" here is equivalent to /SUBSYSTEM:WINDOWS for MSVC
add_executable(nsaur WIN32
        src/game/3DMath.c
        src/game/3dmath.h
        src/game/3DMF.c
        src/game/3dmf.h
        src/game/Bones.c
        src/game/bones.h
        src/game/Camera.c
        src/game/camera.h
        src/game/Collision.c
        src/game/collision.h
        src/game/Effects.c
        src/game/effects.h
        src/game/Enemy.c
        src/game/enemy.h
        src/game/Enemy_Ptera.c
        src/game/Enemy_Rex.c
        src/game/Enemy_Spitter.c
        src/game/Enemy_Stego.c
        src/game/Enemy_TriCer.c
        src/game/EnvironmentMap.c
        src/game/environmentmap.h
        src/game/File.c
        src/game/file.h
        src/game/globals.h
        src/game/HighScores.c
        src/game/highscores.h
        src/game/Infobar.c
        src/game/infobar.h
        src/game/input.c
        src/game/input.h
        src/game/Items.c
        src/game/items.h
        src/game/Main.c
        src/game/main.h
        src/game/MainMenu.c
        src/game/mainmenu.h
        src/game/Misc.c
        src/game/misc.h
        src/game/mobjtypes.h
        src/game/Movie.c
        src/game/movie.h
        src/game/MyGuy.c
        src/game/myguy.h
        src/game/mytraps.h
        src/game/Objects.c
        src/game/objects.h
        src/game/Objects2.c
        src/game/objtypes.h
        src/game/Pickups.c
        src/game/pickups.h
        src/game/Player_Control.c
        src/game/player_control.h
        src/game/QD3D_Geometry.c
        src/game/qd3d_geometry.h
        src/game/qd3d_support.c
        src/game/qd3d_support.h
        src/game/SelfRunDemo.c
        src/game/selfrundemo.h
        src/game/SkeletonAnim.c
        src/game/skeletonanim.h
        src/game/SkeletonJoints.c
        src/game/skeletonjoints.h
        src/game/SkeletonObj.c
        src/game/skeletonobj.h
        src/game/Sound.c
        src/game/sound2.h
        src/game/Sprites.c
        src/game/sprites.h
        src/game/Structs.h
        src/game/terrain.c
        src/game/terrain.h
        src/game/Terrain2.c
        src/game/TimePortal.c
        src/game/timeportal.h
        src/game/Title.c
        src/game/title.h
        src/game/Traps.c
        src/game/Triggers.c
        src/game/triggers.h
        src/game/Weapons.c
        src/game/weapons.h
        src/game/Windows.c
        src/game/windows_nano.h
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
        src/Backdrop.cpp
        src/GamePatches.cpp
        src/GamePatches.h
        src/nsaur.cpp
        src/SettingsScreen.cpp
)

find_package(SDL2 REQUIRED COMPONENTS main)
find_package(OpenGL REQUIRED)

target_compile_definitions(Quesa_SDL PUBLIC QUESA_OS_SDL QUESA_APPLY_FOG_COLOR_IN_ALL_PASSES)
target_compile_definitions(${PROJECT_NAME} PRIVATE SOURCE_PORT_ENHANCEMENTS)

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -include Pomme.h
        -Wall -Wno-multichar
        -fexceptions
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        /FIPomme.h
        /EHa    # C++ may throw exceptions from extern "C" functions
        /wd4068
        /W3
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    src
    src/game
    src/pomme
    src/qd3d
)

target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    Quesa_SDL
    pomme
)

# Copy SDL2 DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endforeach()
endif()