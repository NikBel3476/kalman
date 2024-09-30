cmake_minimum_required(VERSION 3.18)
project(autopilot_selfcheck VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS
	Core
	Gui
	Widgets
	Quick
	SerialPort
	LinguistTools
	SvgWidgets
)

file(GLOB_RECURSE mavlink_sources
	${PROJECT_SOURCE_DIR}/src/mavlink/*.h
)

set(mavlink_lib_name mavlink)

add_library(${mavlink_lib_name} STATIC
	${mavlink_sources}
)

if (MSVC)
	target_compile_options(${mavlink_lib_name} PRIVATE /w)
else()
	target_compile_options(${mavlink_lib_name} PRIVATE -w)
endif()

qt_standard_project_setup(
	I18N_TRANSLATED_LANGUAGES en ru
)

file(GLOB source_files
	${PROJECT_SOURCE_DIR}/src/*.cpp
	${PROJECT_SOURCE_DIR}/src/*.h
	${PROJECT_SOURCE_DIR}/qml/*.qml
)

qt_add_executable(${PROJECT_NAME}
	${source_files}
)

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

qt_add_translations(${PROJECT_NAME}
	TS_FILE_DIR translations
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	WIN32_EXECUTABLE TRUE
	MACOSX_BUNDLE TRUE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::SerialPort
	Qt6::Widgets
	Qt6::Qml
	Qt6::Quick
	Qt6::Platform
	Qt6::SvgWidgets
	mavlink
	qflightinstruments
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/qflightinstruments)

# Resources:
set(resource_files
	"images/application-exit.png"
	"images/clear.png"
	"images/connect.png"
	"images/disconnect.png"
	"images/settings.png"
	"images/refresh.png"
	"images/parameters.png"
	"images/terminal.png"
	"images/reboot.png"
	"qml/AuthenticationForm.qml"
)

qt_add_resources(${PROJECT_NAME} "autopilot_selfcheck"
	PREFIX
	"/"
	FILES
	${resource_files}
)

# set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)

install(TARGETS ${PROJECT_NAME}
	BUNDLE  DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
	TARGET ${PROJECT_NAME}
	OUTPUT_SCRIPT deploy_script
	NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

include(CPack)
