
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 文件归类
file(GLOB SRC_FILES *.h *.cpp)
file(GLOB QT_UI_FILES *.ui)
file(GLOB OCC_FILES	${CMAKE_CURRENT_SOURCE_DIR}/occ/*.*)
file(GLOB READER_FILES	${CMAKE_CURRENT_SOURCE_DIR}/reader/*.*)

source_group(ui FILES ${QT_UI_FILES})
source_group(occ FILES ${OCC_FILES})
source_group(reader FILES ${READER_FILES})

find_package(Qt5 COMPONENTS Widgets Gui REQUIRED)

# opencascade
find_package(OpenCasCade COMPONENTS Kernel REQUIRED)

include_directories(SYSTEM ${OpenCASCADE_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/occ)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/reader)

add_executable(3d_quick_viewer WIN32 ${SRC_FILES} ${QT_UI_FILES} ${OCC_FILES} ${READER_FILES})

# 工程配置
foreach (CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
	if (${CONFIG_TYPE} STREQUAL "Debug")
		foreach (LIB ${OpenCASCADE_LIBRARIES})
			target_link_libraries(3d_quick_viewer debug ${OpenCASCADE_LIBRARY_DIR}/${LIB}.lib)
		endforeach()
	else()
		foreach (LIB ${OpenCASCADE_LIBRARIES})
			target_link_libraries(3d_quick_viewer optimized ${OpenCASCADE_LIBRARY_DIR}/${LIB}.lib)
		endforeach()
	endif()
endforeach()

target_link_libraries(3d_quick_viewer Qt5::Widgets Qt5::Gui)

# 安装
INSTALL(TARGETS 3d_quick_viewer)

file(GLOB OCC_DLLS ${OpenCASCADE_BINARY_DIR}/*.dll)
INSTALL(FILES ${OCC_DLLS} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

INSTALL(FILES copy_to_clipbord.bat DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
