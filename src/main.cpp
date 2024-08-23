/*
 * Copyright (c) 2023 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include ----------------------------------------------------------------- */
#include "Particle.h"

#include "ei_device_particle.h"
#include "ei_at_handlers.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "ei_sensor_imu.h"
#include "ei_microphone.h"
#include "ei_run_impulse.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

/* Constants --------------------------------------------------------------- */
#define CONVERT_G_TO_MS2    9.80665f

/* Private variables ------------------------------------------------------- */
static ATServer *at;

/**
 * @brief      Particle setup function
 */
void setup()
{
    // put your setup code here, to run once:
    EiDeviceParticle *dev = static_cast<EiDeviceParticle*>(EiDeviceInfo::get_device());

    // Wait for serial to make it easier to see the serial logs at startup.
    ei_sleep(2000);
    Serial.begin(115200);
    ei_printf("Edge Impulse inference runner for Particle devices\r\n");

    // Init the sensors
    ei_sensor_imu_init();
    ei_microphone_init();

    // Init AT commands
    at = ei_at_init(dev);
    ei_printf("Type AT+HELP to see a list of commands.\r\n");
    at->print_prompt();
}

void loop()
{
    /* handle command comming from uart */
    char data = ei_getchar();

    if(is_inference_running() == true) {
        if(data == 'b') {
            ei_stop_impulse();
            at->print_prompt();
        }
        else {
            ei_run_impulse();
        }
    }

    else if (data != 0xFF) {
        at->handle(data);
    }
}
