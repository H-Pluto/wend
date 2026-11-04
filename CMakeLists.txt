cmake_minimum_required(VERSION 3.16)
project(MyPet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) # 自动处理 Qt 的信号槽
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS
    resources/*.qrc
)


add_executable(Wend WIN32 ${SOURCES} ${RESOURCES}) # WIN32 表示不弹出黑色控制台窗口
target_link_libraries(Wend PRIVATE Qt6::Widgets)