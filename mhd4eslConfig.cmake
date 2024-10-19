include(CMakeFindDependencyMacro)

find_dependency(esa)
find_dependency(esl)
find_dependency(common4esl)
find_dependency(opengtx4esl)
find_dependency(libmicrohttpd)

include("${CMAKE_CURRENT_LIST_DIR}/mhd4eslTargets.cmake")
