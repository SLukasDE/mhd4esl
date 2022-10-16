cmake_minimum_required(VERSION 3.23)

project(mhd4esl VERSION 1.5.0
            LANGUAGES CXX)
            
#include(FetchContent)
#FetchContent_Declare(
#  catch
#  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#  GIT_TAG        v2.13.6
#)
#FetchContent_MakeAvailable(catch)


# The compiled library code is here
#add_subdirectory(src/main)
# STATIC, SHARED, MODULE, INTERFACE
add_library(mhd4esl STATIC
      ./src/main/mhd4esl/Plugin.cpp
      ./src/main/mhd4esl/Plugin.h
      ./src/main/mhd4esl/Logger.h
      ./src/main/mhd4esl/com/http/server/RequestContext.cpp
      ./src/main/mhd4esl/com/http/server/Socket.cpp
      ./src/main/mhd4esl/com/http/server/Request.cpp
      ./src/main/mhd4esl/com/http/server/Connection.h
      ./src/main/mhd4esl/com/http/server/Connection.cpp
      ./src/main/mhd4esl/com/http/server/RequestContext.h
      ./src/main/mhd4esl/com/http/server/Request.h
      ./src/main/mhd4esl/com/http/server/Socket.h
)

target_include_directories(mhd4esl PUBLIC ./src/main)
target_compile_features(mhd4esl PUBLIC cxx_std_11)
set_property(TARGET mhd4esl PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package(esl REQUIRED)
target_link_libraries(mhd4esl PRIVATE esl::esl)

find_package(common4esl REQUIRED)
target_link_libraries(mhd4esl PRIVATE common4esl::common4esl)

#find_package(LIBMICROHTTPD REQUIRED)
#target_link_libraries(mhd4esl PRIVATE LIBMICROHTTPD::LIBMICROHTTPD)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMICROHTTPD REQUIRED libmicrohttpd)
##pkg_check_modules(WEBSOCKETS REQUIRED libwebsockets)
#target_link_libraries(mhd4esl PRIVATE PkgConfig::PC_MICROHTTPD)
target_link_libraries(mhd4esl PRIVATE microhttpd)

install(TARGETS mhd4esl
        EXPORT mhd4esl-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

install(EXPORT      mhd4esl-export
        FILE        mhd4eslTargets.cmake
        NAMESPACE   mhd4esl::
        DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mhd4eslConfig.cmake
        DESTINATION "lib/cmake"
)




# The compiled test code is here
#add_subdirectory(src/test)
