cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 20)
project(pomme CXX)

set(POMME_SRC
    Pomme.cpp
    Pomme.h
    PommeDebug.cpp
    PommeDebug.h
    PommeEnums.h
    PommeFiles.h
    PommeGraphics.h
    PommeInit.h
    PommeInput.h
    PommeSound.h
    PommeTime.h
    PommeTypes.h
    PommeVideo.h
    Files/ArchiveVolume.cpp
    Files/ArchiveVolume.h
    Files/Files.cpp
    Files/HostVolume.cpp
    Files/HostVolume.h
    Files/Resources.cpp
    Files/Volume.h
    Graphics/ARGBPixmap.cpp
    Graphics/Color.cpp
    Graphics/Graphics.cpp
    Graphics/PICT.cpp
    Graphics/SysFont.h
    Graphics/SystemPalettes.cpp
    Input/SDLInput.cpp
    maconv/stuffit/methods.h
    maconv/stuffit/methods/arsenic.cc
    maconv/stuffit/methods/arsenic.h
    maconv/stuffit/utils/bwt.cc
    maconv/stuffit/utils/bwt.h
    maconv/stuffit/utils/crc.cc
    maconv/stuffit/utils/crc.h
    maconv/utils/bit_reader.cc
    maconv/utils/bit_reader.h
    maconv/utils/bit_reader.h
    Memory/Memory.cpp
    Sound/AIFF.cpp
    Sound/cmixer.cpp
    Sound/cmixer.h
    Sound/IMA4.cpp
    Sound/MACE.cpp
    Sound/SoundManager.cpp
    Sound/xlaw.cpp
    Time/TimeManager.cpp
    Utilities/BigEndianIStream.cpp
    Utilities/FixedPool.h
    Utilities/GrowablePool.h
    Utilities/IEEEExtended.cpp
    Utilities/IEEEExtended.h
    Utilities/memstream.cpp
    Utilities/memstream.h
    Utilities/StringUtils.cpp
    Utilities/StringUtils.h
    Utilities/structpack.cpp
    Utilities/structpack.h
    Video/Cinepak.cpp
    Video/Cinepak.h
    Video/moov.cpp
    $<$<BOOL:${WIN32}>:Platform/Windows/PommeWindows.cpp>
    $<$<BOOL:${WIN32}>:Platform/Windows/PommeWindows.h>
)

add_library(pomme ${POMME_SRC})

find_package(SDL2 REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    .
    ${SDL2_INCLUDE_DIRS}
)

if(MSVC)
    target_compile_options(pomme PRIVATE
        /EHa
    )
else()
    target_compile_options(pomme PRIVATE
        -Wall
        -Wno-multichar
        -fexceptions
    )
endif()
