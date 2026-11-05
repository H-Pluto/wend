cmake_minimum_required(VERSION 3.16)
project(Wend LANGUAGES CXX)

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

# Release 包去掉符号表，体积能小一大半
target_link_options(Wend PRIVATE $<$<CONFIG:Release>:-s>)

install(TARGETS Wend RUNTIME DESTINATION .)

# 打包：安装时跑 windeployqt，把 Qt 运行时 DLL 和插件补到 exe 旁边，
# 得到的目录可以直接压缩分发（目标机器不需要装 Qt）
if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${_qt_bin_dir}")
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "找不到 windeployqt，无法生成可分发目录")
    endif()

    install(CODE "
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --dir \"\${CMAKE_INSTALL_PREFIX}\"
                    --no-translations
                    --no-opengl-sw
                    --no-system-d3d-compiler
                    \"\${CMAKE_INSTALL_PREFIX}/Wend.exe\"
            COMMAND_ERROR_IS_FATAL ANY
        )
    ")
endif()