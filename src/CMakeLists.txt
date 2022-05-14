
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Gui REQUIRED)

file(GLOB SRC_FILES *.h *.cpp)
file(GLOB QT_UI_FILES *.ui)

source_group(ui ${QT_UI_FILES})
add_executable(3d_quick_viewer WIN32 ${SRC_FILES} ${QT_UI_FILES})

target_link_libraries(3d_quick_viewer Qt5::Widgets Qt5::Gui)