# autopilot_selfcheck

## This project based on qt serial terminal example

### Prerequisites

* qt6
* mavlink c library v2
* cmake

### To launch the project

1. configure cmake - `cmake -S . -B build`
2. build - `cmake --build build`
3. run the executable - `./build/autopilot_selfcheck`

#### Note: tested only on linux

### Code formatting
`clang-format -i -style=file *.cpp *.h`