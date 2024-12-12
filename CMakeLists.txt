cmake_minimum_required(VERSION 3.16)
project(autopilot_selfcheck VERSION 0.1.0 LANGUAGES C CXX)

cmake_policy(SET CMP0135 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

include(FetchContent)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(MAVLINK_DIALECT ardupilotmega)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(mavlink mavlink)
set(qflightinstruments qflightinstruments)
set(json json)

FetchContent_Declare(${mavlink}
	GIT_REPOSITORY https://github.com/mavlink/c_library_v2.git
	GIT_TAG 52698eb556445d22ea0f0145fde1ae64719b6646
)
FetchContent_Declare(${qflightinstruments}
	GIT_REPOSITORY https://github.com/NikBel3476/QFlightinstruments.git
)
FetchContent_Declare(${json} URL
	https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(${json} ${qflightinstruments} ${mavlink})

find_package(Qt6 REQUIRED COMPONENTS
	Core
	Gui
	Widgets
	Quick
	SerialPort
	LinguistTools
	SvgWidgets
	Concurrent
)

qt_standard_project_setup(
	I18N_TRANSLATED_LANGUAGES en ru
)

file(GLOB source_files
	${PROJECT_SOURCE_DIR}/src/*.cpp
	${PROJECT_SOURCE_DIR}/src/*.hpp
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

target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION="${PROJECT_VERSION}")
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
	Qt6::Concurrent
	qflightinstruments
	nlohmann_json::nlohmann_json
)

# SYSTEM PRIVATE to suppress warnings
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
	${mavlink_SOURCE_DIR}
)

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
	"images/folder.png"
	"images/firmware.png"
	"images/about.png"
	"qml/AuthenticationForm.qml"
)

qt_add_resources(${PROJECT_NAME} "autopilot_selfcheck"
	PREFIX
	"/"
	FILES
	${resource_files}
)

# set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)

install(CODE [[
	file(GET_RUNTIME_DEPENDENCIES)
]])

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
