
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB SRC_FILES *.h *.cpp)
file(GLOB QT_UI_FILES *.ui)

source_group(ui ${QT_UI_FILES})

find_package(Qt5 COMPONENTS Widgets Gui REQUIRED)

# opencascade
find_package(OpenCasCade COMPONENTS Kernel REQUIRED)

set(OCC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/occ/OcctWindow.h
	${CMAKE_CURRENT_SOURCE_DIR}/occ/OcctWindow.cxx
)
source_group(occ ${OCC_FILES})

include_directories(SYSTEM ${OpenCASCADE_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/occ)

add_executable(3d_quick_viewer ${SRC_FILES} ${QT_UI_FILES} ${OCC_FILES})

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

INSTALL(TARGETS 3d_quick_viewer)

file(GLOB OCC_FILES ${OpenCASCADE_BINARY_DIR}/*.dll)
INSTALL(FILES ${OCC_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)