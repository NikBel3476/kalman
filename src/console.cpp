#include "console.hpp"

#include <QScrollBar>

Console::Console(QWidget *parent, MavlinkManager *mavlink_manager)
		: QPlainTextEdit(parent),
			_mavlink_manager(mavlink_manager) {
	setWindowTitle(tr("Mavlink messages"));
	// document()->setMaximumBlockCount(100);
	QPalette p = palette();
	p.setColor(QPalette::Base, Qt::black);
	p.setColor(QPalette::Text, Qt::green);
	setPalette(p);

	// connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&Console::_handleMavlinkMessageReceive);
}

void Console::putData(const QByteArray &data) {
	insertPlainText(data);

	QScrollBar *bar = verticalScrollBar();
	bar->setValue(bar->maximum());
}

void Console::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	const auto msg =
			std::format("ID: {} sequence: {} from component: {} of system: {}\n",
									std::to_string(mavlink_message.msgid),
									std::to_string(mavlink_message.seq),
									std::to_string(mavlink_message.compid),
									std::to_string(mavlink_message.sysid));
	QByteArray data(msg.c_str(), static_cast<uint32_t>(msg.length()));
	putData(data);

	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_HEARTBEAT: {
		mavlink_heartbeat_t heartbeat;
		mavlink_msg_heartbeat_decode(&mavlink_message, &heartbeat);
		const auto heartbeat_str = std::format(
				"HEARTBEAT type: {} autopilot: {} base_mode: {} custom_mode: {} "
				"system_status: {} mavlink_version: {}\n",
				heartbeat.type, heartbeat.autopilot, heartbeat.base_mode,
				heartbeat.custom_mode, heartbeat.system_status,
				heartbeat.mavlink_version);
		QByteArray data(heartbeat_str.c_str(),
										static_cast<uint32_t>(heartbeat_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_SYS_STATUS: {
		mavlink_sys_status_t sys_status;
		mavlink_msg_sys_status_decode(&mavlink_message, &sys_status);
		const auto status_str = std::format(
				"sensors present: {:b} sensors enabled: {:b} load: {} errors_comm: "
				"{}\n",
				static_cast<uint32_t>(sys_status.onboard_control_sensors_present),
				static_cast<uint32_t>(sys_status.onboard_control_sensors_enabled),
				std::to_string(sys_status.voltage_battery),
				std::to_string(sys_status.errors_comm));
		QByteArray data(status_str.c_str(),
										static_cast<uint32_t>(status_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
		mavlink_global_position_int_t global_position;
		mavlink_msg_global_position_int_decode(&mavlink_message, &global_position);
		const auto coords_str =
				std::format("lon: {} lat: {} alt: {} vx: {} vy: {} vz: {} hdg: {}\n",
										global_position.lon, global_position.lat,
										global_position.alt, global_position.vx, global_position.vy,
										global_position.vz, global_position.hdg);
		QByteArray data(coords_str.c_str(),
										static_cast<uint32_t>(coords_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_POWER_STATUS: {
		mavlink_power_status_t power_status;
		mavlink_msg_power_status_decode(&mavlink_message, &power_status);
		const auto status_str =
				std::format("Rail voltage: {} mV\n", power_status.Vcc);
		QByteArray data(status_str.c_str(),
										static_cast<uint32_t>(status_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_RAW_IMU: {
		mavlink_raw_imu_t raw_imu;
		mavlink_msg_raw_imu_decode(&mavlink_message, &raw_imu);
		const auto raw_imu_str = std::format(
				"Time: {} xacc: {} yacc: {} zacc: {} xgyro: {} ygyro: {} zgyro: {} "
				"xmag: {} ymag: {} zmag: {} id: {}\n",
				std::to_string(raw_imu.time_usec), std::to_string(raw_imu.xacc),
				std::to_string(raw_imu.yacc), std::to_string(raw_imu.zacc),
				std::to_string(raw_imu.xgyro), std::to_string(raw_imu.ygyro),
				std::to_string(raw_imu.zgyro), std::to_string(raw_imu.xmag),
				std::to_string(raw_imu.ymag), std::to_string(raw_imu.zmag),
				std::to_string(raw_imu.id));
		QByteArray data(raw_imu_str.c_str(),
										static_cast<uint32_t>(raw_imu_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_MCU_STATUS: {
		mavlink_mcu_status_t mcu_status;
		mavlink_msg_mcu_status_decode(&mavlink_message, &mcu_status);
		auto mcu_status_str =
				std::format("MCU_STATUS temp: {} voltage: {} v_min: {} v_max: {}\n",
										mcu_status.MCU_temperature, mcu_status.MCU_voltage,
										mcu_status.MCU_voltage_min, mcu_status.MCU_voltage_max);
		QByteArray data(mcu_status_str.c_str(),
										static_cast<uint32_t>(mcu_status_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_COMMAND_ACK: {
		mavlink_command_ack_t cmd_ack;
		mavlink_msg_command_ack_decode(&mavlink_message, &cmd_ack);
		auto cmd_ack_str = std::format(
				"CMD_ACK COMMAND: {} RESULT: {} PROGRESS: {} RES_PRM2: {} "
				"TARGET_SYS: {} TARGET_CMP: {}\n",
				cmd_ack.command, cmd_ack.result, cmd_ack.progress,
				cmd_ack.result_param2, cmd_ack.target_system, cmd_ack.target_component);
		QByteArray data(cmd_ack_str.c_str(),
										static_cast<uint32_t>(cmd_ack_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_COMMAND_LONG: {
		mavlink_command_long_t cmd;
		mavlink_msg_command_long_decode(&mavlink_message, &cmd);
		auto cmd_str = std::format(
				"CMD_LONG COMMAND: {} SYSTEM: {} COMPONENT: {} CONFIRMATION: {} "
				"param1: {} param2: {} param3: {} param4: {} param5: "
				"{} param6: {} param7: {}\n",
				cmd.command, cmd.target_system, cmd.target_component, cmd.confirmation,
				cmd.param1, cmd.param2, cmd.param3, cmd.param4, cmd.param5, cmd.param6,
				cmd.param7);
		QByteArray data(cmd_str.c_str(), static_cast<uint32_t>(cmd_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_PARAM_VALUE: {
		mavlink_param_value_t param_value;
		mavlink_msg_param_value_decode(&mavlink_message, &param_value);
		auto param_value_str = std::format(
				"PARAM_VALUE ID: {} VALUE: {} TYPE: {} COUNT: {} INDEX: {}\n",
				param_value.param_id, param_value.param_value, param_value.param_type,
				param_value.param_count, param_value.param_index);
		QByteArray data(param_value_str.c_str(),
										static_cast<uint32_t>(param_value_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_STATUSTEXT: {
		mavlink_statustext_t statustext;
		mavlink_msg_statustext_decode(&mavlink_message, &statustext);
		auto statustext_str =
				std::format("STATUSTEXT ID: {} CHUNK_SEQ: {} SEVERITY: {} TEXT: {}\n",
										std::to_string(statustext.id), statustext.chunk_seq,
										statustext.severity, statustext.text);
		QByteArray data(statustext_str.c_str(),
										static_cast<uint32_t>(statustext_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_MAG_CAL_PROGRESS: {
		mavlink_mag_cal_progress_t mag_cal_progress;
		mavlink_msg_mag_cal_progress_decode(&mavlink_message, &mag_cal_progress);
		auto mag_cal_progress_str =
				std::format("MAG_CAL_PROGRESS ATTEMPT: {} PCT: {} STATUS: {}\n",
										mag_cal_progress.attempt, mag_cal_progress.completion_pct,
										mag_cal_progress.cal_status);
		QByteArray data(mag_cal_progress_str.c_str(),
										static_cast<uint32_t>(mag_cal_progress_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_MAG_CAL_REPORT: {
		mavlink_mag_cal_report_t mag_cal_report;
		mavlink_msg_mag_cal_report_decode(&mavlink_message, &mag_cal_report);
		auto mag_cal_report_str =
				std::format("MAG_CAL_REPORT STATUS: {}", mag_cal_report.cal_status);
		QByteArray data(mag_cal_report_str.c_str(),
										static_cast<uint32_t>(mag_cal_report_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_SCALED_IMU: {
		mavlink_scaled_imu_t scaled_imu;
		mavlink_msg_scaled_imu_decode(&mavlink_message, &scaled_imu);
		auto scaled_imu_str =
				std::format("SCALED_IMU xacc: {} yacc: {} zacc: {} xgyro: {} "
										"ygyro: {} zgyro: {} xmag: {} ymag: {} zmag: {}\n",
										scaled_imu.xacc, scaled_imu.yacc, scaled_imu.zacc,
										scaled_imu.xgyro, scaled_imu.ygyro, scaled_imu.zgyro,
										scaled_imu.xmag, scaled_imu.ymag, scaled_imu.zmag);
		QByteArray data(scaled_imu_str.c_str(),
										static_cast<uint32_t>(scaled_imu_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_SCALED_IMU2: {
		mavlink_scaled_imu2_t scaled_imu;
		mavlink_msg_scaled_imu2_decode(&mavlink_message, &scaled_imu);
		auto scaled_imu_str =
				std::format("SCALED_IMU2 xacc: {} yacc: {} zacc: {} xgyro: {} "
										"ygyro: {} zgyro: {} xmag: {} ymag: {} zmag: {}\n",
										scaled_imu.xacc, scaled_imu.yacc, scaled_imu.zacc,
										scaled_imu.xgyro, scaled_imu.ygyro, scaled_imu.zgyro,
										scaled_imu.xmag, scaled_imu.ymag, scaled_imu.zmag);
		QByteArray data(scaled_imu_str.c_str(),
										static_cast<uint32_t>(scaled_imu_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_SCALED_IMU3: {
		mavlink_scaled_imu3_t scaled_imu;
		mavlink_msg_scaled_imu3_decode(&mavlink_message, &scaled_imu);
		auto scaled_imu_str =
				std::format("SCALED_IMU3 xacc: {} yacc: {} zacc: {} xgyro: {} "
										"ygyro: {} zgyro: {} xmag: {} ymag: {} zmag: {}\n",
										scaled_imu.xacc, scaled_imu.yacc, scaled_imu.zacc,
										scaled_imu.xgyro, scaled_imu.ygyro, scaled_imu.zgyro,
										scaled_imu.xmag, scaled_imu.ymag, scaled_imu.zmag);
		QByteArray data(scaled_imu_str.c_str(),
										static_cast<uint32_t>(scaled_imu_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_ATTITUDE: {
		mavlink_attitude_t attitude;
		mavlink_msg_attitude_decode(&mavlink_message, &attitude);
		auto attitude_str = std::format(
				"ATTITUDE roll: {} pitch: {} yaw: {} rollspeed: {} pitchspeed: {} "
				"yawspeed: {}\n",
				attitude.roll, attitude.pitch, attitude.yaw, attitude.rollspeed,
				attitude.pitchspeed, attitude.yawspeed);
		QByteArray data(attitude_str.c_str(),
										static_cast<uint32_t>(attitude_str.length()));
		putData(data);
	} break;
	case MAVLINK_MSG_ID_VFR_HUD: {
		mavlink_vfr_hud_t vfr_hud;
		mavlink_msg_vfr_hud_decode(&mavlink_message, &vfr_hud);
		auto vfr_hud_str =
				std::format("VFR_HUD airspeed: {} groundspeed: {} heading: {} "
										"throttle: {} alt: {} climb: {}",
										vfr_hud.airspeed, vfr_hud.groundspeed, vfr_hud.heading,
										vfr_hud.throttle, vfr_hud.alt, vfr_hud.climb);
		QByteArray data(vfr_hud_str.c_str(),
										static_cast<uint32_t>(vfr_hud_str.length()));
		putData(data);
	} break;
	default:
		break;
	}
}
