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

> Tested only on linux at this moment

### Code formatting command
`clang-format -i -style=file *.cpp *.h`


## Info about mavlink messages for calibration

GS - ground station, AP - autopilot  
<SYSTEM_ID> = 255  
<COMPONENT_ID> = 190 (MAV_COMP_ID_MISSIONPLANNER)  

### Accelerometer calibration description
Accelerometer calibration begins with calibration request from ground station
1. Preflight calibration request | GS -> AP
```
   msg: COMMAND_LONG (76)  
   target_system: 1
   target_component: 1	
   command: MAV_CMD_PREFLIGHT_CALIBRATION (241)
   confirmation: 0
   param1: 0
   param2: 0
   param3: 0
   param4: 0
   param5: 1 
   param6: 0
   param7: 0
```

2. After calibration start begins the seqence of messages that tells in what position to 
put the vehicle in. After each such message, you need to send a reply message after 
setting the autopilot to the required position. I hadn't found the certain message, it 
seems to be works with different types of them, for example QGroundControl sends 
following:  
COMMAND_ACK (77)  
system_id: 255  
component_id: MAV_COMP_ID_MISSIONPLANNER (190)
```
command: 0
result: MAV_RESULT_TEMPORARILY_REJECTED
progress: 0
result_param: 0
target_system: 0
target_component: 0
```

**Message template for both AP -> GS and GS -> AP**  
COMMAND_LONG (76)  
```
target_system: <SYSTEM_ID>
target_component: <COMPONENT_ID>
command: MAV_CMD_ACCELCAL_VEHICLE_POS (42429)
confirmation: 0
param1: <position>
param2: 0
param3: 0
param4: 0
param5: 0 
param6: 0
param7: 0
```

For messages AP -> GS  
SYSTEM_ID: 255  
COMPONENT_ID: 190  

For messages GS -> AP  
SYSTEM_ID: 1
COMPONENT_ID: 1

param1 (position) value sequence:
1. ACCELCAL_VEHICLE_POS_LEVEL (1)
2. ACCELCAL_VEHICLE_POS_LEFT (2)
3. ACCELCAL_VEHICLE_POS_RIGHT (3)
4. ACCELCAL_VEHICLE_POS_NOSEDOWN (4)
5. ACCELCAL_VEHICLE_POS_NOSEUP (5)
6. ACCELCAL_VEHICLE_POS_BACK (6)
7. ACCELCAL_VEHICLE_POS_SUCCESS (16777215) - if calibration process completed  
   ACCELCAL_VEHICLE_POS_FAILED (16777216) - if something went wrong during calibration  

After successful calibration the autopilot must be restarted