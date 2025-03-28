# autopilot_selfcheck

Program for autopilot calibration

## This project based on qt serial terminal example

### Prerequisites

* git
* qt 6.5 or above
* cmake 3.16 or above
* gcc 13 or clang 15 (or above versions) (for linux)
* microsoft visual studio 2022 (for windows)

### Project building

#### Cmake
1. clone the repository - `git clone <repository_url>`
2. configure cmake - `cmake -B build`
3. build - `cmake --build build --parallel --config Release`
4. run the executable - `./build/autopilot_selfcheck`

To create archive:  
`cpack --config "build/CPackConfig.cmake"` - Linux  
`cpack --config "build/CPackConfig.cmake" -G ZIP` - Windows

Note: on windows qt must be in the PATH variable or passed with `-DCMAKE_PREFIX_PATH=<path_to_qt>` on configuration step

> Tested on linux and windows at this moment

#### Docker (WIP)
1. Build docker image - `docker build -t autopilot_selfcheck/autopilotselfcheck:0.1 .`
2. Start building inside container - WIP

### Development setup
#### Prerequisites
* clang-format
* pre-commit (command for installation: `pip install pre-commit`)

After repository cloning run command: `pre-commit install`
Now on every `git commit` command it will format all modified c++ files,
if format returns failed it reports that files have been formatted, run
`git add` and `git commit` again

### Code formatting command
`clang-format -i -style=file src/*.cpp src/*.hpp`

## Info about mavlink messages for calibration

GS - ground station, AP - autopilot  
<SYSTEM_ID> = 255  
<COMPONENT_ID> = 190 (MAV_COMP_ID_MISSIONPLANNER)  

### Accelerometer calibration description
Accelerometer calibration begins with calibration request from ground station
1. Preflight calibration request GS -> AP
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
put the vehicle in.  
After each such message, you need to send a reply message after 
setting the autopilot to the required position. I hadn't found the certain message, it 
seems to be works with different types of them, for example QGroundControl sends 
following:  
```
msg: COMMAND_ACK (77)  
system_id: 255  
component_id: MAV_COMP_ID_MISSIONPLANNER (190)
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

### Magnetometer calibration description
Magnetometer calibration begins with calibration request from ground station
1. Magnetometer calibration request GS -> AP
```
msg: COMMAND_LONG (76)
target_sytem: 1
target_component: 1
command: MAV_CMD_DO_START_MAG_CAL
confirmation: 0
param1 = 0; // Bitmask (all)
param2 = 1; // Retry on failure
param3 = 1; // Autosave
param4 = 0; // Delay
param5 = 0; // Autoreboot
param6 = 0;
param7 = 0;
```

2. After calibration start begins the sequence of messages that shows 
the progress of calibration
AP -> GS
```
msg: MAG_CAL_PROGRESS (191)
compass_id: 0 (all)
cal_status: MAG_CAL_STATUS
completion_pct: <completion percentage>
...
```

> MAG_CAL_STATUS = MAG_CAL_RUNNING_STEP_ONE or MAG_CAL_RUNNING_STEP_TWO

3. If calibration completed succesfully then autopilot send following message  
AP -> GS
```
msg: MAG_CAL_REPORT (192)
compass_id: 0 (all)
cal_status: MAG_CAL_SUCCESS
...
```

After successful calibration the autopilot must be restarted

### Gyroscope calibration description
Gyroscope calibration begins with calibration request from ground station
1. Preflight calibration request GS -> AP
```
   msg: COMMAND_LONG (76)  
   target_system: 1
   target_component: 1	
   command: MAV_CMD_PREFLIGHT_CALIBRATION (241)
   confirmation: 0
   param1: 1
   param2: 0
   param3: 0
   param4: 0
   param5: 0
   param6: 0
   param7: 0
```
2. Wait for COMMAND_ACK message with calibration result
```
msg: COMMAND_ACK (77)
system_id: 1 
component_id: 1
command: MAV_CMD_PREFLIGHT_CALIBRATION (241)
result: MAV_RESULT_ACCEPTED
progress: 0
result_param: 0
target_system: 255
target_component: 190
```

### Autopilot parameters
GS -> AP  
PARAM_REQUEST_LIST (21) - request all parameter list  
PARAM_SET (23) - set parameter on autopilot

AP -> GS  
PARAM_VALUE (22) - parameter from autopilot  
> After parameters set autopilot will return written parameters but with index 65535

> Note: on startup autopilot do not send all available parameters, so to get all of them
you need to wait ~10-15 seconds

### Firmware upload

> Attention: do not plug more than one fc board to pc at firmware upload due to app will
try to reboot all available serial devices and can upload firmware to wrong board

### MavFTP

lua scripts are uploaded using the following algorithm:
1. List directory `/APM`
2. If `/APM` does not contain `scripts` directory create it and go to 4-th step
3. List `/APM/scripts` directory and remove all files inside
4. Create and write all files to AP

