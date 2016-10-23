cmake_minimum_required(VERSION 3.6)

# Platform specific code
if (UNIX)
    message("Unix system. Setting GCC flags.")

    set(CMAKE_C_COMPILER gcc-6)
    set(CMAKE_CXX_COMPILER g++-6)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -std=c99 -Wall -DSDL -DTARGET_UNIX -DNDEBUG -DNO_ASSEMBLY")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=c++14 -Wall -DSDL -DTARGET_UNIX -DNDEBUG -DNO_ASSEMBLY")
elseif (WIN32)
    message("Win32 system. Setting MSVC flags.")
    #TODO

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc-")
endif ()

project(AmuletsArmor)
enable_language(C CXX)

include_directories(include/amulets)

# SDL2
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
find_package(SDL2_net REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_NET_INCLUDE_DIR})

set(SOURCES
        src/SDL/ipx_client.cpp
        src/3D_COLLI.c
        src/3D_IO.c
        src/3D_TRIG.c
        src/3D_VIEW.c
        src/ACTIVITY.c
        src/AREASND.c
        src/BANKUI.c
        src/BANNER.c
        src/BUTTON.c
        src/CLI_RECV.c
        src/CLI_SEND.c
        src/CLIENT.c
        src/CMDQUEUE.c
        src/COLOR.c
        src/COLORIZE.c
        src/COMWIN.c
        src/CONFIG.c
        src/CONTROL.c
        src/CRELOGIC.c
        src/CSYNCPCK.c
        src/DBLLINK.c
        src/DEBUG.c
        src/DITALK.c
        src/DOOR.c
        src/EFFECT.c
        src/EFX.c
        src/ESCMENU.c
        src/FILE.c
        src/FORM.c
        src/GRAPHIC.c
        src/GRAPHICS_SDL.c
        src/GUILDUI.c
        src/HARDFORM.c
        src/HOUSEUI.c
        src/INIFILE.c
        src/INNUI.c
        src/INVENTOR.c
        src/KEYBOARD_SDL.c
        src/KEYMAP.c
        src/LIGHT.c
        src/LOOK.c
        src/MAINUI.c
        src/MAP.c
        src/MAPANIM.c
        src/MEMORY.c
        src/MESSAGE.c
        src/MOUSEMOD.c
        src/NOTES.c
        src/OBJECT.c
        src/OBJGEN.c
        src/OBJMOVE.c
        src/OBJTYPE.c
        src/OVERHEAD.c
        src/OVERLAY.c
        src/PACKETDT.c
        src/PACKETPR.c
        src/PEOPHERE.c
        src/PICS.c
        src/PLAYER.c
        src/PROMPT.c
        src/RANDOM.c
        src/RESOURCE.c
        src/SCHEDULE.c
        src/SCRFORM.c
        src/SCRIPT.c
        src/SERVER.c
        src/SERVERSH.c
        src/SLIDER.c
        src/SLIDR.c
        src/SMACHINE.c
        src/SMCCHOOS.c
        src/SMCPLAY.c
        src/SMMAIN.c
        src/SOSEZ.c
        src/SOUND.c
        src/SPELLS.c
        src/SQRTDAT.c
        src/STATS.c
        src/STORE.c
        src/SYNCMEM.c
        src/SYNCTIME.c
        src/TESTME.c
        src/TEXT.c
        src/TICKER_SDL.c
        src/TOWNUI.c
        src/TXTBOX.c
        src/TXTFLD.c
        src/UI.c
        src/UIBUTTON.c
        src/UITEXT.c
        src/UPDATE.c
        src/VIEW.c
        src/VM.c
        Build/SDL2/main.cc
        Build/SDL2/sdl_engine.cc
        Build/SDL2/sdl_extern.cc
        src/helpers.c
        src/platform/deserialization.c
        )

add_executable(AmuletsArmor ${SOURCES})
target_link_libraries(AmuletsArmor "${SDL2_LIBRARY} ${SDL2_NET_LIBRARY}")
set_target_properties(AmuletsArmor PROPERTIES RUNTIME_OUTPUT_DIRECTORY /tmp/aa/)
set_target_properties(AmuletsArmor
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

message(${SDL2_INCLUDE_DIR})
message(${SDL2_NET_INCLUDE_DIR})

set_property(TARGET AmuletsArmor APPEND_STRING PROPERTY COMPILE_FLAGS " -O0 -g -Wno-ignored-attributes")

add_custom_command(TARGET AmuletsArmor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:AmuletsArmor>)