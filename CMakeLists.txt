cmake_minimum_required(VERSION 3.23)
project(3DQuickViewer)
add_subdirectory(src)


INSTALL(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
