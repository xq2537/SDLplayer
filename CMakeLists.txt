cmake_minimum_required (VERSION 2.6)

project ( AndroVMplayer )
set ( EXECUTABLE_OUTPUT_PATH out/ )
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

find_package ( Threads )

include_directories (
  includes
  /usr/include/SDL
  SDL/include
  /libSDL/include
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  ADD_LIBRARY(sdlmac STATIC IMPORTED)
  SET_PROPERTY(TARGET sdlmac PROPERTY IMPORTED_LOCATION lib/mac64/libSDL.a)
  ADD_LIBRARY(sdlmainmac STATIC IMPORTED)
  SET_PROPERTY(TARGET sdlmainmac PROPERTY IMPORTED_LOCATION lib/mac64/libSDLmain.a)
  SET(SDL_LIBRARY sdlmac)
  SET(SDL_MAINLIBRARY sdlmainmac)
  SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-framework,OpenGL -Wl,-framework,Cocoa -Wl,-framework,ApplicationServices -Wl,-framework,Carbon -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit -Wl,-framework,IOKit")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  ADD_LIBRARY(sdllinux32 STATIC IMPORTED)
  SET_PROPERTY(TARGET sdllinux32 PROPERTY IMPORTED_LOCATION lib/linux32/libSDL.a)
  ADD_LIBRARY(sdlmainlinux32 STATIC IMPORTED)
  SET_PROPERTY(TARGET sdlmainlinux32 PROPERTY IMPORTED_LOCATION lib/linux32/libSDLmain.a)
  SET(SDL_LIBRARY sdllinux32)
  SET(SDL_MAINLIBRARY sdlmainlinux32)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  ADD_LIBRARY(sdlwin32 STATIC IMPORTED)
  SET_PROPERTY(TARGET sdlwin32 PROPERTY IMPORTED_LOCATION lib/win32/SDL.lib)
  SET(SDL_LIBRARY sdlwin32)
  #ADD_LIBRARY(sdlmainwin32 STATIC IMPORTED)
  #SET_PROPERTY(TARGET sdlmainwin32 PROPERTY IMPORTED_LOCATION lib/win32/SDLmain.lib)
  #SET(SDL_MAINLIBRARY sdlmainwin32)
endif()

ADD_EXECUTABLE (
  AndroVMplayer
  ./srcs/main.c
  ./srcs/sensor_listener.c
  ./srcs/misc.c
  ./srcs/adb.c
  ./srcs/socket.c
)

ADD_LIBRARY ( libOpenglRender SHARED IMPORTED )

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET_TARGET_PROPERTIES ( libOpenglRender PROPERTIES IMPORTED_LOCATION lib/lib64OpenglRender.dylib )
  SET(LIB_OPENGLRENDER "libOpenglRender")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(CFLAG_ARCHI "-m32")
  SET_TARGET_PROPERTIES ( libOpenglRender PROPERTIES IMPORTED_LOCATION lib/libOpenglRender.so )
  SET(LIB_OPENGLRENDER "libOpenglRender")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  SET(CFLAG_ARCHI "-m32")
  SET_TARGET_PROPERTIES ( libOpenglRender PROPERTIES IMPORTED_LOCATION lib/OpenglRender )
  SET(LIB_OPENGLRENDER "libOpenglRender")
  FIND_LIBRARY( WINSOCK_LIB WS2_32 PATHS "C:\\Program Files\\Microsoft\ SDKs\\Windows\\v7.1\\Lib" )
endif()

TARGET_LINK_LIBRARIES (
  AndroVMplayer
  ${SDL_LIBRARY}
  ${SDL_MAINLIBRARY}
  ${LIB_OPENGLRENDER}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_SDL_LIBS_INIT}
  ${WINSOCK_LIB}
)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
SET (
  CMAKE_C_FLAGS
  "-g -W -Wall ${CFLAG_ARCHI}"
)
endif()
