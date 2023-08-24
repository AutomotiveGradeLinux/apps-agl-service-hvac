/*
 * Copyright (C) 2022,2023 Konsulko Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HVAC_SERVICE_H
#define _HVAC_SERVICE_H

#include <glib.h>

#include "KuksaConfig.h"
#include "KuksaClient.h"
#include "HvacCanHelper.h"
#include "HvacLedHelper.h"

class HvacService
{
public:
	HvacService(const KuksaConfig &config, GMainLoop *loop = NULL);

	~HvacService();

	// Callback for KuksaClient subscribe API reconnect

	static gboolean resubscribe_cb(gpointer data) {
		struct resubscribe_data *d = (struct resubscribe_data*) data;
		if (d && d->self) {
			((HvacService*) d->self)->Resubscribe(d->request);
		}
		return FALSE;
	}

private:
	struct resubscribe_data {
		HvacService *self;
		const SubscribeRequest *request;
	};

	GMainLoop *m_loop;
	KuksaConfig m_config;
	KuksaClient *m_broker;
	HvacCanHelper m_can_helper;
	HvacLedHelper m_led_helper;

	void HandleSignalChange(const std::string &path, const Datapoint &dp);

	void HandleSignalSetError(const std::string &path, const Error &error);

	void HandleSubscribeDone(const SubscribeRequest *request, const Status &status);

	void Resubscribe(const SubscribeRequest *request);

	void set_left_temperature(uint8_t temp);

	void set_right_temperature(uint8_t temp);

	void set_left_fan_speed(uint8_t temp);

	void set_right_fan_speed(uint8_t temp);

	void set_fan_speed(uint8_t temp);
};

#endif // _HVAC_SERVICE_H
