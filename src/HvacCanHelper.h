/*
 * Copyright (C) 2022,2023 Konsulko Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HVAC_CAN_HELPER_H
#define _HVAC_CAN_HELPER_H

#include <cstdint>
#include <string>
#include <linux/can.h>

class HvacCanHelper
{
public:
	HvacCanHelper(bool verbose = false);

	~HvacCanHelper();

	void set_left_temperature(uint8_t temp);

	void set_right_temperature(uint8_t temp);

	void set_fan_speed(uint8_t temp);

private:
	uint8_t convert_temp(uint8_t value) {
		int result = ((0xF0 - 0x10) / 15) * (value - 15) + 0x10;
		if (result < 0x10)
			result = 0x10;
		if (result > 0xF0)
			result = 0xF0;

		return (uint8_t) result;
	}

	void read_config();

	void can_open();

	void can_close();

	void can_update();

	std::string m_port = "can0";
	unsigned m_verbose = 0;
	bool m_config_valid = false;
	bool m_active = false;
	int m_can_socket;
	struct sockaddr_can m_can_addr;

	uint8_t m_temp_left = 21;
	uint8_t m_temp_right = 21;
	uint8_t m_fan_speed = 0;
};

#endif // _HVAC_CAN_HELPER_H
