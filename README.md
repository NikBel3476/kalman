# autopilot_selfcheck

## This project based on qt serial terminal example

### Prerequisites

* qt6
* cmake 3.18 or above
* microsoft visual studio 2022 (for windows)

### To launch the project

1. clone the repository - `git clone --recursive <repository_url>`
2. configure cmake - `cmake -B build`
3. build - `cmake --build build`
4. run the executable - `./build/autopilot_selfcheck`

Note: on windows qt must be in the PATH variable or passed with `-DCMAKE_PREFIX_PATH=<path_to_qt>` on configuration step

#### Note: tested only on linux

### Code formatting
`clang-format -i -style=file *.cpp *.h`
