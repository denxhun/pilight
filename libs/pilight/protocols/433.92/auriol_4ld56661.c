/*
	Copyright (C) 2021 denxhun

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "auriol_4ld56661.h"

#define RAW_LENGTH	52
#define MID_WIDTH	1500

typedef struct settings_t {
    double id;
    double temp;
    double rain;
    double battery_ok;
    struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static void parseCode(void) {
	int i = 0, x = 0, binary[RAW_LENGTH/2];
	int channel = 0, id = 0, battery = 0;
	double temp_offset = 0.0, temperature = 0.0, rain = 0.0;

	if(auriol_4ld56661->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "auriol_4ld56661: parsecode - invalid length parameter passed %d", auriol_4ld56661->rawlen);
		return;
	}

	for(x=1;x<auriol_4ld56661->rawlen-2;x+=2) {
		if(auriol_4ld56661->raw[x] > MID_WIDTH) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	id = binToDecRev(binary, 0, 7);
	battery_ok = binToDecRev(binary,8,11) == 0x8;
	temperature = (double)binToSignedRev(binary, 12, 23)/10;
	rain = (double)binToSignedRev(binary, 28, 51)*0.3;
/*
	struct settings_t *tmp = settings;
	while(tmp) {
		if(fabs(tmp->id-id) < EPSILON) {
			temp_offset = tmp->temp;
			break;
		}
		tmp = tmp->next;
	}

	temperature += temp_offset;

	if(channel != 4) {
		auriol_4ld56661->message = json_mkobject();
		json_append_member(auriol_4ld56661->message, "id", json_mknumber(id, 0));
		json_append_member(auriol_4ld56661->message, "temperature", json_mknumber(temperature, 1));
		json_append_member(auriol_4ld56661->message, "battery", json_mknumber(battery, 0));
		json_append_member(auriol_4ld56661->message, "channel", json_mknumber(channel, 0));
	}*/
}

static int checkValues(struct JsonNode *jvalues) {
	struct JsonNode *jid = NULL;

	if((jid = json_find_member(jvalues, "id"))) {
		struct settings_t *snode = NULL;
		struct JsonNode *jchild = NULL;
		struct JsonNode *jchild1 = NULL;
		double id = -1;
		int match = 0;

		jchild = json_first_child(jid);
		while(jchild) {
			jchild1 = json_first_child(jchild);
			while(jchild1) {
				if(strcmp(jchild1->key, "id") == 0) {
					id = jchild1->number_;
				}
				jchild1 = jchild1->next;
			}
			jchild = jchild->next;
		}

		struct settings_t *tmp = settings;
		while(tmp) {
			if(fabs(tmp->id-id) < EPSILON) {
				match = 1;
				break;
			}
			tmp = tmp->next;
		}

		if(match == 0) {
			if((snode = MALLOC(sizeof(struct settings_t))) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			snode->id = id;
			snode->temp = 0;

			json_find_number(jvalues, "temperature-offset", &snode->temp);

			snode->next = settings;
			settings = snode;
		}
	}
	return 0;
}

static void gc(void) {
	struct settings_t *tmp = NULL;
	while(settings) {
		tmp = settings;
		settings = settings->next;
		FREE(tmp);
	}
	if(settings != NULL) {
		FREE(settings);
	}
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void auriol_4ld56661Init(void) {

	protocol_register(&auriol_4ld56661);
	protocol_set_id(auriol_4ld56661, "auriol_4ld56661");
	protocol_device_add(auriol_4ld56661, "auriol_4ld56661", "Auriol 4-LD56661 Weather Stations");
	auriol_4ld56661->devtype = WEATHER;
	auriol_4ld56661->hwtype = RF433;
	auriol_4ld56661->minrawlen = RAW_LENGTH;
	auriol_4ld56661->maxrawlen = RAW_LENGTH;
	auriol_4ld56661->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	auriol_4ld56661->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&auriol_4ld56661->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
	options_add(&auriol_4ld56661->options, "b", "battery-ok", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[01]$");
	options_add(&auriol_4ld56661->options, "t", "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,3}$");
 	options_add(&auriol_4ld56661->options, "r", "rain", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,8}$");

	options_add(&auriol_4ld56661->options, "0", "temperature-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&auriol_4ld56661->options, "0", "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&auriol_4ld56661->options, "0", "rain-offset", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&auriol_4ld56661->options, "0", "rain-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "[0-9]");
	options_add(&auriol_4ld56661->options, "0", "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&auriol_4ld56661->options, "0", "show-rain", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&auriol_4ld56661->options, "0", "show-battery", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");

	auriol_4ld56661->parseCode=&parseCode;
	auriol_4ld56661->checkValues=&checkValues;
	auriol_4ld56661->validate=&validate;
	auriol_4ld56661->gc=&gc;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "auriol_4ld56661";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	auriol_4ld56661Init();
}
#endif
