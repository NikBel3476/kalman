# autopilot_selfcheck

## This project based on qt serial terminal example

### Prerequisites

* qt6
* cmake
* microsoft visual studio 2022 (for windows)

### To launch the project

1. cmake configuration - `cmake -B build`
2. build - `cmake --build build`
3. run the executable - `./build/autopilot_selfcheck`

Note: on windows qt must be in the PATH variable or passed with `-DCMAKE_PREFIX_PATH=<path_to_qt>` on configuration step

#### Note: tested only on linux

### Code formatting
`clang-format -i -style=file *.cpp *.h`
