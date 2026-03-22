cmake_minimum_required(VERSION 3.21)
project(FemEmStudio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets OpenGLWidgets)

qt_add_executable(FemEmStudio
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ViewportWidget.h
    src/ViewportWidget.cpp
    src/FemCoreBridge.h
    src/FemCoreBridge.cpp
    src/SimulationConfig.h
)

target_link_libraries(FemEmStudio PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

target_compile_definitions(FemEmStudio PRIVATE
    QT_NO_KEYWORDS
)
