# autopilot_selfcheck

## This project based on qt serial terminal example

### Prerequisites

* qt6
* mavlink c library v2 (ardupilotmega dialect)
* cmake
* microsoft visual studio 2022 (for windows)

### To launch the project

1. cmake configuration - `cmake -S . -B build`
2. build - `cmake --build build`
3. run the executable - `./build/autopilot_selfcheck`

Note: on windows qt and mavlink must be in the PATH variable or passed with `-DCMAKE_PREFIX_PATH=<path_to_qt>;<path_to_mavlink>` on configuration step

#### Note: tested only on linux

### Code formatting
`clang-format -i -style=file *.cpp *.h`
