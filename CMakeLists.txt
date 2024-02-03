cmake_minimum_required(VERSION 3.5)

project(zoomme)

# Find the required Qt modules
find_package(Qt6 COMPONENTS Core Gui OpenGL Widgets OpenGLWidgets REQUIRED)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Set up the target
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_FLAGS "-ggdb")

set(TARGET    zoomme) # Executable name
set(SOURCES   main.cpp zoomwidget.cpp)
set(HEADERS   zoomwidget.hpp)
set(UI        zoomwidget.ui)
set(RESOURCES resources.qrc)

add_executable(${TARGET} ${SOURCES} ${HEADERS} ${UI} ${RESOURCES})

# Link against the required Qt modules
target_link_libraries(zoomme
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
