/*
 * Copyright (C) 2022,2023,2025 Konsulko Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HvacLedHelper.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <toml.hpp>

#define RED "/sys/class/leds/blinkm-3-9-red/brightness"
#define GREEN "/sys/class/leds/blinkm-3-9-green/brightness"
#define BLUE "/sys/class/leds/blinkm-3-9-blue/brightness"

// RGB temperature mapping
static struct {
	const int temperature;
	const int rgb[3];
} degree_colours[] = {
	{15, {0, 0, 229} },
	{16, {22, 0, 204} },
	{17, {34, 0, 189} },
	{18, {46, 0, 175} },
	{19, {58, 0, 186} },
	{20, {70, 0, 146} },
	{21, {82, 0, 131} },
	{22, {104, 0, 116} },
	{23, {116, 0, 102} },
	{24, {128, 0, 87} },
	{25, {140, 0, 73} },
	{26, {152, 0, 58} },
	{27, {164, 0, 43} },
	{28, {176, 0, 29} },
	{29, {188, 0, 14} },
	{30, {201, 0, 5} }
};


HvacLedHelper::HvacLedHelper()
{
	read_config();
}

void HvacLedHelper::read_config()
{
	// Using a separate configuration file now, it may make sense
	// to revisit this if a workable scheme to handle overriding
	// values for the full demo setup can be come up with.
	std::string configFile = "";
	char *home = getenv("XDG_CONFIG_HOME");
	if (home) {
		configFile = home;
		configFile += "/AGL/agl-service-hvac/leds.toml";

		if (!std::filesystem::exists(std::filesystem::status(configFile)))
			configFile.clear();
	}
	if (configFile.empty())
		configFile = "/etc/xdg/AGL/agl-service-hvac/leds.toml";
	std::cout << "Reading configuration " << configFile << std::endl;
	toml::value config;
	try {
		config = toml::parse(configFile);
	} catch(const toml::syntax_error& err) {
		std::cerr << "Syntax error in " << configFile << std::endl;

		// Report all the errors
		std::cerr << err.what() << std::endl;

		// Continue with defaults if file missing/broken
		m_config_valid = true;
		return;
	} catch(const std::exception& ex) {
		std::cerr << "Could not read " << configFile << std::endl;

		// Continue with defaults if file missing/broken
		m_config_valid = true;
		return;
	}

	m_led_path_red = toml::find_or(config, "red", RED);
	std::stringstream ss;
	ss << m_led_path_red;
	ss >> std::quoted(m_led_path_red);
	if (m_led_path_red.empty()) {
		std::cerr << "Invalid red LED path" << std::endl;
		return;
	}
	// stat file here?
	std::cout << "Using red LED path " << m_led_path_red << std::endl;

	m_led_path_green = toml::find_or(config, "green", GREEN);
	std::stringstream().swap(ss);
	ss << m_led_path_green;
	ss >> std::quoted(m_led_path_green);
	if (m_led_path_green.empty()) {
		std::cerr << "Invalid green LED path" << std::endl;
		return;
	}
	// stat file here?
	std::cout << "Using green LED path " << m_led_path_red << std::endl;

	m_led_path_blue = toml::find_or(config, "blue", BLUE);
	std::stringstream().swap(ss);
	ss << m_led_path_blue;
	ss >> std::quoted(m_led_path_blue);
	if (m_led_path_blue.empty()) {
		std::cerr << "Invalid blue LED path" << std::endl;
		return;
	}
	// stat file here?
	std::cout << "Using blue LED path " << m_led_path_red << std::endl;

	m_config_valid = true;
}

void HvacLedHelper::set_left_temperature(uint8_t temp)
{
	m_temp_left = temp;
	led_update();
}

void HvacLedHelper::set_right_temperature(uint8_t temp)
{
	m_temp_right = temp;
	led_update();
}

void HvacLedHelper::led_update()
{
	if (!m_config_valid)
		return;

	// Calculates average colour value taken from the temperature toggles,
	// limiting to our 15 degree range
	int temp_left = m_temp_left - 15;
	if (temp_left < 0)
		temp_left = 0;
	else if (temp_left > 15)
		temp_left = 15;

	int temp_right = m_temp_right - 15;
	if (temp_right < 0)
		temp_right = 0;
	else if (temp_right > 15)
		temp_right = 15;

	int red_value = (degree_colours[temp_left].rgb[0] + degree_colours[temp_right].rgb[0]) / 2;
	int green_value = (degree_colours[temp_left].rgb[1] + degree_colours[temp_right].rgb[1]) / 2;
	int blue_value = (degree_colours[temp_left].rgb[2] + degree_colours[temp_right].rgb[2]) / 2;

	//
	// Push colour mapping out
	//

	std::ofstream led_red;
	led_red.open(m_led_path_red);
	if (led_red.is_open()) {
		led_red << std::to_string(red_value);
		led_red.close();
	} else {
		std::cerr << "Could not write red LED path " << m_led_path_red << std::endl;
		m_config_valid = false;
		return;
	}

	std::ofstream led_green;
	led_green.open(m_led_path_green);
	if (led_green.is_open()) {
		led_green << std::to_string(green_value);
		led_green.close();
	} else {
		std::cerr << "Could not write green LED path " << m_led_path_green << std::endl;
		m_config_valid = false;
		return;
	}

	std::ofstream led_blue;
	led_blue.open(m_led_path_blue);
	if (led_blue.is_open()) {
		led_blue << std::to_string(blue_value);
		led_blue.close();
	} else {
		std::cerr << "Could not write blue LED path " << m_led_path_blue << std::endl;
		m_config_valid = false;
		return;
	}
}
