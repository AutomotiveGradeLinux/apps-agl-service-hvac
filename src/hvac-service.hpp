// SPDX-License-Identifier: Apache-2.0

#ifndef _HVAC_SERVICE_HPP
#define _HVAC_SERVICE_HPP

#include "vis-session.hpp"
#include "hvac-can-helper.hpp"
#include "hvac-led-helper.hpp"

class HvacService : public VisSession
{
public:
	HvacService(const VisConfig &config, net::io_context& ioc, ssl::context& ctx);

protected:
	virtual void handle_authorized_response(void) override;

	virtual void handle_get_response(std::string &path, std::string &value, std::string &timestamp) override;

	virtual void handle_notification(std::string &path, std::string &value, std::string &timestamp) override;

private:
	HvacCanHelper m_can_helper;
	HvacLedHelper m_led_helper;

	void set_left_temperature(uint8_t temp);

	void set_right_temperature(uint8_t temp);

	void set_fan_speed(uint8_t temp);
};

#endif // _HVAC_SERVICE_HPP
