// SPDX-License-Identifier: Apache-2.0

#include "hvac-service.hpp"
#include <iostream>
#include <algorithm>


HvacService::HvacService(const VisConfig &config, net::io_context& ioc, ssl::context& ctx) :
	VisSession(config, ioc, ctx),
	m_can_helper(),
	m_led_helper()
{
}

void HvacService::handle_authorized_response(void)
{
	subscribe("Vehicle.Cabin.HVAC.Station.Row1.Left.Temperature");
	subscribe("Vehicle.Cabin.HVAC.Station.Row1.Left.FanSpeed");
	subscribe("Vehicle.Cabin.HVAC.Station.Row1.Right.Temperature");
	subscribe("Vehicle.Cabin.HVAC.Station.Row1.Right.FanSpeed");
}

void HvacService::handle_get_response(std::string &path, std::string &value, std::string &timestamp)
{
	// Placeholder since no gets are performed ATM
}

void HvacService::handle_notification(std::string &path, std::string &value, std::string &timestamp)
{
	if (path == "Vehicle.Cabin.HVAC.Station.Row1.Left.Temperature") {
		try {
			int temp = std::stoi(value);
			if (temp >= 0 && temp < 256)
				set_left_temperature(temp);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} else if (path == "Vehicle.Cabin.HVAC.Station.Row1.Right.Temperature") {
		try {
			int temp = std::stoi(value);
			if (temp >= 0 && temp < 256)
				set_right_temperature(temp);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} else if (path == "Vehicle.Cabin.HVAC.Station.Row1.Left.FanSpeed") {
		try {
			int speed = std::stoi(value);
			if (speed >= 0 && speed < 256)
				set_fan_speed(speed);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} else if (path == "Vehicle.Cabin.HVAC.Station.Row1.Right.FanSpeed") {
		try {
			int speed = std::stoi(value);
			if (speed >= 0 && speed < 256)
				set_fan_speed(speed);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	}
	// else ignore
}

void HvacService::set_left_temperature(uint8_t temp)
{
	m_can_helper.set_left_temperature(temp);
	m_led_helper.set_left_temperature(temp);
}

void HvacService::set_right_temperature(uint8_t temp)
{
	m_can_helper.set_right_temperature(temp);
	m_led_helper.set_right_temperature(temp);
}

void HvacService::set_fan_speed(uint8_t speed)
{
	m_can_helper.set_fan_speed(speed);
}
