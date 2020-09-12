cmake_minimum_required(VERSION 3.17)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "C_as_CXX.cmake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQUESA_OS_SDL -DQUESA_APPLY_FOG_COLOR_IN_ALL_PASSES -DSOURCE_PORT_ENHANCEMENTS -Wno-multichar -Wno-deprecated")

project(nsaur CXX)

add_subdirectory(Quesa)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        src
        src/game
        src/pomme
        src/pomme/Sound
        src/qd3d
)

add_definitions(-include Pomme.h)

add_executable(nsaur
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
        src/pomme/Files/Volume.h
        src/pomme/Files/HostVolume.cpp
        src/pomme/Files/HostVolume.h
        src/pomme/Files/ArchiveVolume.cpp
        src/pomme/Files/ArchiveVolume.h
        src/pomme/maconv/stuffit/methods.h
        src/pomme/maconv/stuffit/methods/arsenic.cc
        src/pomme/maconv/stuffit/methods/arsenic.h
        src/pomme/maconv/stuffit/utils/bwt.cc
        src/pomme/maconv/stuffit/utils/bwt.h
        src/pomme/maconv/stuffit/utils/crc.cc
        src/pomme/maconv/stuffit/utils/crc.h
        src/pomme/maconv/utils/bit_reader.h
        src/pomme/maconv/utils/bit_reader.cc
        src/pomme/maconv/utils/bit_reader.h
        src/pomme/Sound/AIFF.cpp
        src/pomme/Sound/cmixer.cpp
        src/pomme/Sound/cmixer.h
        src/pomme/Sound/DumpAU.h
        src/pomme/Sound/IMA4.cpp
        src/pomme/Sound/MACE.cpp
        src/pomme/Sound/SoundManager.cpp
        src/pomme/ARGBPixmap.cpp
        src/pomme/BigEndianIStream.cpp
        src/pomme/Files.cpp
        src/pomme/FixedPool.h
        src/pomme/Graphics.cpp
        src/pomme/GrowablePool.h
        src/pomme/IEEEExtended.c
        src/pomme/IEEEExtended.h
        src/pomme/Memory.cpp
        src/pomme/PascalStringHack.h
        src/pomme/PICT.cpp
        src/pomme/Pomme.cpp
        src/pomme/Pomme.h
        src/pomme/PommeDebug.cpp
        src/pomme/PommeDebug.h
        src/pomme/PommeEnums.h
        src/pomme/PommeInternal.h
        src/pomme/PommeTypes.h
        src/pomme/QD3D.cpp
        src/pomme/Resources.cpp
        src/pomme/structpack.cpp
        src/pomme/structpack.h
        src/pomme/SysFont.h
        src/pomme/TimeManager.cpp
        src/pomme/WinInput.cpp
        src/pomme/memstream.cpp
        src/pomme/memstream.h
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
        src/StructUnpackers.cpp)

target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARY}
        ${SDL2_LIBRARIES}
        Quesa_SDL
        )
