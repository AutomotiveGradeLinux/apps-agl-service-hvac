/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Copyright (C) 2016, 2020 Konsulko Group
 * Author "Romain Forlot"
 * Author "Jose Bolo"
 * Author "Scott Murray <scott.murray@konsulko.com>"
 * Author "Matt Ranostay <matt.ranostay@konsulko.com>"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

#define RED "/sys/class/leds/blinkm-3-9-red/brightness"
#define GREEN "/sys/class/leds/blinkm-3-9-green/brightness"
#define BLUE "/sys/class/leds/blinkm-3-9-blue/brightness"

static afb_event_t event;

/*****************************************************************************************/
/*****************************************************************************************/
/**											**/
/**											**/
/**	   SECTION: HANDLE CAN DEVICE							**/
/**											**/
/**											**/
/*****************************************************************************************/
/*****************************************************************************************/

// Initialize CAN hvac array that will be sent trough the socket
static struct {
	const char *name;
	uint8_t value;
} hvac_values[] = {
	{ "LeftTemperature", 21 },
	{ "RightTemperature", 21 },
	{ "Temperature", 21 },
	{ "FanSpeed", 0 },
	{ "ACEnabled", 0 },
	{ "LeftLed", 15 },
	{ "RightLed", 15 }
};

// Holds RGB combinations for each temperature
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

struct led_paths {
	const char *red;
	const char *green;
	const char *blue;
}led_paths_values = {
	.red = RED,
	.green = GREEN,
	.blue = BLUE
};

// Get original get temperature function from cpp hvacplugin code
static uint8_t to_can_temp(uint8_t value)
{
	int result = ((0xF0 - 0x10) / 15) * (value - 15) + 0x10;
	if (result < 0x10)
		result = 0x10;
	if (result > 0xF0)
		result = 0xF0;

	return (uint8_t)result;
}

static uint8_t read_temp_left_zone()
{
	return hvac_values[0].value;
}

static uint8_t read_temp_right_zone()
{
	return hvac_values[1].value;
}

static uint8_t read_temp_left_led()
{
	return hvac_values[5].value;
}

static uint8_t read_temp_right_led()
{
	return hvac_values[6].value;
}

static uint8_t read_temp()
{
	return (uint8_t)(((int)read_temp_left_zone() + (int)read_temp_right_zone()) >> 1);
}

static uint8_t read_fanspeed()
{
	return hvac_values[3].value;
}

/* 
 * @param: None
 *
 * @brief: Parse JSON configuration file for blinkm path
 */
static int parse_config()
{
	struct json_object *ledtemp = NULL;
	struct json_object *jobj = json_object_from_file("/etc/hvac.json");

	// Check if .json file has been parsed as a json_object
	if (!jobj) {
		AFB_ERROR("JSON file could not be opened!\n");
		return 1;
	}

	// Check if json_object with key "ledtemp" has been found in .json file
	if (!json_object_object_get_ex(jobj, "ledtemp", &ledtemp)){
		AFB_ERROR("Key not found!\n");
		return 1;
	}

	// Extract directory paths for each LED colour
	json_object_object_foreach(ledtemp, key, value) {
		if (strcmp(key, "red") == 0) {
			led_paths_values.red = json_object_get_string(value);
		} else if (strcmp(key, "green") == 0) {
			led_paths_values.green = json_object_get_string(value);
		} else if (strcmp(key, "blue") == 0) {
			led_paths_values.blue = json_object_get_string(value);
		}
	}

	return 0;
}

/* 
 * @brief Writing to LED for both temperature sliders
 */
static int temp_write_led()
{

	int red_value, green_value, blue_value;
	int right_temp;
	int left_temp;

	left_temp = read_temp_left_led() - 15;
	right_temp = read_temp_right_led() - 15;

	// Calculates average colour value taken from the temperature toggles
	red_value = (degree_colours[left_temp].rgb[0] + degree_colours[right_temp].rgb[0]) / 2;
	green_value = (degree_colours[left_temp].rgb[1] + degree_colours[right_temp].rgb[1]) / 2;
	blue_value = (degree_colours[left_temp].rgb[2] + degree_colours[right_temp].rgb[2]) / 2;
	
	// default path: /sys/class/leds/blinkm-3-9-red/brightness
	FILE* r = fopen(led_paths_values.red, "w");
	if(r){
		fprintf(r, "%d", red_value);
		fclose(r);
	} else {
		AFB_ERROR("Unable to open red LED path!\n");
		return -1;
	}

	// default path: /sys/class/leds/blinkm-3-9-green/brightness
	FILE* g = fopen(led_paths_values.green, "w");
	if(g){
		fprintf(g, "%d", green_value);
		fclose(g);
	} else {
		AFB_ERROR("Unable to open green LED path!\n");
		return -1;
	}

	// default path: /sys/class/leds/blinkm-3-9-blue/brightness
	FILE* b = fopen(led_paths_values.blue, "w");
	if(b){
		fprintf(b, "%d", blue_value);
		fclose(b);
	} else {
		AFB_ERROR("Unable to open blue LED path!\n");
		return -1;
	}

	return 0;
}

/*
 * @brief Get temperature of left toggle in HVAC system
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void temp_left_zone_led(afb_req_t request)
{
	int i = 5, rc, x, changed;
	double d;
	struct json_object *query, *val;
	uint8_t values[sizeof hvac_values / sizeof *hvac_values];
	uint8_t saves[sizeof hvac_values / sizeof *hvac_values];

	AFB_WARNING("In temp_left_zone_led.");

	query = afb_req_json(request);

	/* records initial values */
	AFB_WARNING("Records initial values");
	values[i] = saves[i] = hvac_values[i].value;


	if (json_object_object_get_ex(query, hvac_values[i].name, &val))
	{
		AFB_WARNING("Value of values[i] = %d", values[i]);
		AFB_WARNING("We got it. Tests if it is an int or double.");
		if (json_object_is_type(val, json_type_int)) {
			x = json_object_get_int(val);
			AFB_WARNING("We get an int: %d",x);
		}
		else if (json_object_is_type(val, json_type_double)) {
			d = json_object_get_double(val);
			x = (int)round(d);
			AFB_WARNING("We get a double: %f => %d",d,x);
		}
		else {
			afb_req_fail_f(request, "bad-request",
				"argument '%s' isn't integer or double", hvac_values[i].name);
			return;
		}
		if (x < 0 || x > 255)
		{
			afb_req_fail_f(request, "bad-request",
				"argument '%s' is out of bounds", hvac_values[i].name);
			return;
		}
		if (values[i] != x) {
			values[i] = (uint8_t)x;
			changed = 1;
			AFB_WARNING("%s changed to %d", hvac_values[i].name,x);
		}
	}
	else {
		AFB_WARNING("%s not found in query!",hvac_values[i].name);
	}


	if (changed) {
		hvac_values[i].value = values[i]; // update structure at line 102
		AFB_WARNING("WRITE_LED: value: %d", hvac_values[i].value);
		rc = temp_write_led();
		if (rc >= 0) {
			afb_req_success(request, NULL, NULL);
			return;
		}
		/* restore initial values */
		hvac_values[i].value = saves[i];
		afb_req_fail(request, "error", "I2C error");
	}
}

/*
 * @brief Get temperature of right toggle in HVAC system
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void temp_right_zone_led(afb_req_t request)
{
	int i = 6, rc, x, changed;
	double d;
	struct json_object *query, *val;
	uint8_t values[sizeof hvac_values / sizeof *hvac_values];
	uint8_t saves[sizeof hvac_values / sizeof *hvac_values];

	AFB_WARNING("In temp_right_zone_led.");

	query = afb_req_json(request);

	/* records initial values */
	AFB_WARNING("Records initial values");
	values[i] = saves[i] = hvac_values[i].value;


	if (json_object_object_get_ex(query, hvac_values[i].name, &val))
	{
		AFB_WARNING("Value of values[i] = %d", values[i]);
		AFB_WARNING("We got it. Tests if it is an int or double.");
		if (json_object_is_type(val, json_type_int)) {
			x = json_object_get_int(val);
			AFB_WARNING("We get an int: %d",x);
		}
		else if (json_object_is_type(val, json_type_double)) {
			d = json_object_get_double(val);
			x = (int)round(d);
			AFB_WARNING("We get a double: %f => %d",d,x);
		}
		else {
			afb_req_fail_f(request, "bad-request",
				"argument '%s' isn't integer or double", hvac_values[i].name);
			return;
		}
		if (x < 0 || x > 255)
		{
			afb_req_fail_f(request, "bad-request",
				"argument '%s' is out of bounds", hvac_values[i].name);
			return;
		}
		if (values[i] != x) {
			values[i] = (uint8_t)x;
			changed = 1;
			AFB_WARNING("%s changed to %d", hvac_values[i].name,x);
		}
	}
	else {
		AFB_WARNING("%s not found in query!", hvac_values[i].name);
	}


	if (changed) {
		hvac_values[i].value = values[i]; // update structure at line 102
		AFB_WARNING("WRITE_LED: value: %d", hvac_values[i].value);

		rc = temp_write_led();
		if (rc >= 0) {
			afb_req_success(request, NULL, NULL);
			return;
		}

		/* restore initial values */
		hvac_values[i].value = saves[i];
		afb_req_fail(request, "error", "I2C error");
	}
}

static int write_can(afb_api_t api)
{
	json_object *jresp = json_object_new_object();
	json_object *jobj = json_object_new_object();
	json_object *jarray = json_object_new_array();

	json_object_object_add(jresp, "bus_name", json_object_new_string("ls"));
	json_object_object_add(jresp, "frame", jobj);

	json_object_object_add(jobj, "can_id", json_object_new_int(0x30));
	json_object_object_add(jobj, "can_dlc", json_object_new_int(8));


	json_object_array_add(jarray,
		json_object_new_int(to_can_temp(read_temp_left_zone())));

	json_object_array_add(jarray,
		json_object_new_int(to_can_temp(read_temp_right_zone())));

	json_object_array_add(jarray,
		json_object_new_int(to_can_temp(read_temp())));

	json_object_array_add(jarray, json_object_new_int(0xf0));
	json_object_array_add(jarray, json_object_new_int(read_fanspeed()));
	json_object_array_add(jarray, json_object_new_int(1));
	json_object_array_add(jarray, json_object_new_int(0));
	json_object_array_add(jarray, json_object_new_int(0));

	json_object_object_add(jobj, "can_data", jarray);

	return afb_api_call_sync(api, "low-can", "write", jresp, NULL, NULL, NULL);
}

/*****************************************************************************************/
/*****************************************************************************************/
/**											**/
/**											**/
/**	   SECTION: BINDING VERBS IMPLEMENTATION					**/
/**											**/
/**											**/
/*****************************************************************************************/
/*****************************************************************************************/

/*
 * @brief Get fan speed HVAC system
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void get_fanspeed(afb_req_t request)
{
	json_object *ret_json;
	uint8_t fanspeed = read_fanspeed();

	ret_json = json_object_new_object();
	json_object_object_add(ret_json, "FanSpeed", json_object_new_int(fanspeed));

	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Read Consign right zone temperature for HVAC system
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void get_temp_right_zone(afb_req_t request)
{
	json_object *ret_json;
	uint8_t temp = read_temp_right_zone();

	ret_json = json_object_new_object();
	json_object_object_add(ret_json, "RightTemperature", json_object_new_int(temp));

	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Read Consign left zone temperature for HVAC system
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void get_temp_left_zone(afb_req_t request)
{
	json_object *ret_json;
	uint8_t temp = read_temp_left_zone();

	ret_json = json_object_new_object();
	json_object_object_add(ret_json, "LeftTemperature", json_object_new_int(temp));

	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Read all values
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void get(afb_req_t request)
{
	AFB_REQ_DEBUG(request, "Getting all values");
	json_object *ret_json;

	ret_json = json_object_new_object();
	json_object_object_add(ret_json, "LeftTemperature", json_object_new_int(read_temp_left_zone()));
	json_object_object_add(ret_json, "RightTemperature", json_object_new_int(read_temp_right_zone()));
	json_object_object_add(ret_json, "FanSpeed", json_object_new_int(read_fanspeed()));

	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Set a component value using a json object retrieved from request
 *
 * @param afb_req_t : pointer to a afb request structure
 *
 */
static void set(afb_req_t request)
{
	int i, rc, x, changed;
	double d;
	struct json_object *query, *val;
	uint8_t values[sizeof hvac_values / sizeof *hvac_values];
	uint8_t saves[sizeof hvac_values / sizeof *hvac_values];
	afb_api_t api = afb_req_get_api(request);

	/* records initial values */
	AFB_DEBUG("Records initial values");
	i = (int)(sizeof hvac_values / sizeof *hvac_values);
	while (i) {
		i--;
		values[i] = saves[i] = hvac_values[i].value;
	}

	/* Loop getting arguments */
	query = afb_req_json(request);
	changed = 0;
	i = (int)(sizeof hvac_values / sizeof *hvac_values);
	AFB_DEBUG("Looping for args. i: %d", i);
	while (i)
	{
		i--;
		AFB_DEBUG("Searching... query: %s, i: %d, comp: %s", json_object_to_json_string(query), i, hvac_values[i].name);
		if (json_object_object_get_ex(query, hvac_values[i].name, &val))
		{
			AFB_DEBUG("We got it. Tests if it is an int or double.");
			if (json_object_is_type(val, json_type_int)) {
				x = json_object_get_int(val);
				AFB_DEBUG("We get an int: %d",x);
			}
			else if (json_object_is_type(val, json_type_double)) {
				d = json_object_get_double(val);
				x = (int)round(d);
				AFB_DEBUG("We get a double: %f => %d",d,x);
			}
			else {
				afb_req_fail_f(request, "bad-request",
					"argument '%s' isn't integer or double", hvac_values[i].name);
				return;
			}
			if (x < 0 || x > 255)
			{
				afb_req_fail_f(request, "bad-request",
					"argument '%s' is out of bounds", hvac_values[i].name);
				return;
			}
			if (values[i] != x) {
				values[i] = (uint8_t)x;
				changed = 1;
				AFB_DEBUG("%s changed to %d",hvac_values[i].name,x);
			}
		}
		else {
			AFB_DEBUG("%s not found in query!",hvac_values[i].name);
		}
	}

	/* attemps to set new values */
	AFB_DEBUG("Diff: %d", changed);
	if (changed)
	{
		i = (int)(sizeof hvac_values / sizeof *hvac_values);
		while (i) {
			i--;
			hvac_values[i].value = values[i];
		}
		rc = write_can(api);
		if (rc >= 0) {
			afb_req_success(request, NULL, NULL);
			return;
		}

		/* restore initial values */
		i = (int)(sizeof hvac_values / sizeof *hvac_values);
		while (i) {
			i--;
			hvac_values[i].value = saves[i];
		}
		afb_req_fail(request, "error", "CAN error");
	}
	else {
		afb_req_success(request, NULL, "No changes");
	}
}

static int bindingServicePreInit(afb_api_t api)
{
	if(parse_config() != 0)
		AFB_WARNING("Default values are being used!\n");

	return 0;
}

static int bindingServiceInit(afb_api_t api)
{
	event = afb_daemon_make_event("language");

	if(afb_daemon_require_api("identity", 1))
		return -1;

	if(afb_daemon_require_api("low-can", 1))
		return -1;

	if (afb_api_call_sync(api, "low-can", "auth", NULL, NULL, NULL, NULL))
		return -1;

	return afb_api_call_sync(api, "identity", "subscribe", json_object_new_object(), NULL, NULL, NULL);
}

static void onEvent(afb_api_t api, const char *event_name, struct json_object *object)
{
	json_object *id_evt_name, *current_identity;

	if (json_object_object_get_ex(object, "eventName", &id_evt_name) &&
	  !strcmp(json_object_get_string(id_evt_name), "login") &&
	  !afb_api_call_sync(api, "identity", "get", json_object_new_object(), &current_identity, NULL, NULL)) {
		json_object *language = NULL;
		json_object *response;
		if (! json_object_object_get_ex(current_identity, "response", &response) || ! json_object_object_get_ex(response, "graphPreferredLanguage", &language)) {
			language = json_object_new_string("en_US");
		}
		afb_event_broadcast(event, language);
		return;
	}
}

// TODO: Have to change session management flag to AFB_SESSION_CHECK to use token auth
static const afb_verb_t hvac_verbs[]= {
	{
		.verb = "get_temp_left_zone",
		.callback = get_temp_left_zone,
		.info = "Get the left zone temperature",
		.session = AFB_SESSION_NONE,
	},
	{
		.verb = "get_temp_right_zone",
		.callback = get_temp_right_zone,
		.info = "Get the right zone temperature",
		.session = AFB_SESSION_NONE,
	},
	{
		.verb = "get_fanspeed",
		.callback = get_fanspeed,
		.info = "Read fan speed",
		.session = AFB_SESSION_NONE,
	},
	{
		.verb = "get",
		.callback = get,
		.info = "Read all speed",
		.session = AFB_SESSION_NONE,
	},
	{
		.verb = "set",
		.callback = set,
		.info = "Set a HVAC component value",
		.session = AFB_SESSION_NONE,
	},
	{
		.verb = "temp_left_zone_led",
		.callback = temp_left_zone_led,
		.info = "Turn on LED on left temperature zone",
		.session = AFB_SESSION_NONE,
	},
	{
		.verb = "temp_right_zone_led",
		.callback = temp_right_zone_led,
		.info = "Turn on LED on left temperature zone",
		.session = AFB_SESSION_NONE,

	},
	{ }
};

const afb_binding_t afbBindingV3 = {
    .api = "hvac",
    .info = "HVAC service API",
    .verbs = hvac_verbs,
    .preinit = bindingServicePreInit,
    .init = bindingServiceInit,
    .onevent = onEvent,
};
