cmake_minimum_required(VERSION 3.25)

project(
    FreewaySimulatorQt
    VERSION 1.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(
    Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        PrintSupport
)

set(QCUSTOMPLOT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qcustomplot"
)

add_executable(FreewaySimulatorQt WIN32
    src/main.cpp

    src/MainWindow.cpp
    src/MainWindow.h

    src/Simulation.cpp
    src/Simulation.h

    src/FreewayView.cpp
    src/FreewayView.h

    ${QCUSTOMPLOT_DIR}/qcustomplot.cpp
    ${QCUSTOMPLOT_DIR}/qcustomplot.h
)

target_include_directories(
    FreewaySimulatorQt
    PRIVATE
        src
        ${QCUSTOMPLOT_DIR}
)

target_link_libraries(
    FreewaySimulatorQt
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
)

if(WIN32)
    get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)

    get_filename_component(
        QT_BIN_DIR
        "${QMAKE_EXECUTABLE}"
        DIRECTORY
    )

    add_custom_command(
        TARGET FreewaySimulatorQt
        POST_BUILD

        COMMAND
            "${QT_BIN_DIR}/windeployqt.exe"
            --$<IF:$<CONFIG:Debug>,debug,release>
            --no-translations
            "$<TARGET_FILE:FreewaySimulatorQt>"

        COMMENT
            "Deploying Qt runtime files..."
    )
endif()