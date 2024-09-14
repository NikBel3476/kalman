cmake_minimum_required(VERSION 3.18)
project(autopilot_selfcheck LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Quick
    SerialPort
    LinguistTools
)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES ru)

qt_add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp mainwindow.h
    console.cpp console.h
    AuthenticationForm.qml
    authenticationform.h authenticationform.cpp
    firmwareuploadpage.h firmwareuploadpage.cpp
    magnetometerinfowidget.h magnetometerinfowidget.cpp
    autopilotsettingspage.h autopilotsettingspage.cpp
    accelerometerinfowidget.h accelerometerinfowidget.cpp
    gyroscopeinfowidget.h gyroscopeinfowidget.cpp
    mcuinfowidget.h mcuinfowidget.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

qt_add_translations(${PROJECT_NAME} TS_FILES autopilot_selfcheck_en.ts autopilot_selfcheck_ru.ts)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::SerialPort
    Qt::Widgets
    Qt6::Qml
    Qt6::Quick
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mavlink)

# Resources:
set(resource_files
    "images/application-exit.png"
    "images/clear.png"
    "images/connect.png"
    "images/disconnect.png"
    "images/settings.png"
    "AuthenticationForm.qml"
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
