cmake_minimum_required(VERSION 3.16)
project(autopilot_selfcheck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Qt6 REQUIRED COMPONENTS Core Gui SerialPort Widgets)
find_package(MAVLink REQUIRED)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    console.cpp console.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::SerialPort
    Qt::Widgets
    MAVLink::mavlink
)

# Resources:
set(resource_files
    "images/application-exit.png"
    "images/clear.png"
    "images/connect.png"
    "images/disconnect.png"
    "images/settings.png"
)

qt_add_resources(${PROJECT_NAME} "autopilot_selfcheck"
    PREFIX
        "/"
    FILES
        ${resource_files}
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
