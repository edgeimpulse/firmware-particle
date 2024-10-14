/*
 * Copyright (c) 2023 Edge Impulse Inc.
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
#include "ei_sensor_imu.h"
#include "ei_device_particle.h"
#include "Particle.h"
#include "ADXL362DMA.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include <iostream>
#include <vector>
/* Constants --------------------------------------------------------------- */
#define CONVERT_G_TO_MS2    9.80665f

/* Private variables ------------------------------------------------------- */
ADXL362DMA *accel;
static float sample_buffer[N_SENSOR_AXES];
sampler_callback inertial_cb_sampler;


bool ei_sensor_imu_init(void)
{
    const uint8_t status_timeout = 10;
    uint8_t stat_read_count = 0;

    /* Init & configure sensor */
    accel = new ADXL362DMA(SPI, D13 /* A2 */);

    accel->softReset();
    delay(100);
    while(accel->readStatus() == 0 && stat_read_count++ < status_timeout) {
        ei_printf("no status yet, waiting for accelerometer\r\n");
        ei_sleep(1000);
    }

    if(stat_read_count >= status_timeout) {
        ei_printf("Failed to read status from accelerometer\r\n");
        return false;
    }
    else {
        accel->writeFilterControl(accel->RANGE_2G, false, false, accel->ODR_200);
        accel->setMeasureMode(true);

        ei_add_sensor_to_fusion_list(imu_sensor);

        return true;
    }
}

float *ei_sensor_imu_read_data(int n_samples)
{
    int16_t acc[3];

    accel->readXYZ(acc[0], acc[1], acc[2]);

    for (int i = 0; i < 3; i++) {
        sample_buffer[i] = (((float)(acc[i] * 2)) / 2048.f) * CONVERT_G_TO_MS2;
    }

    return &sample_buffer[0];
}

void ei_accel_read_data(void)
{
    inertial_cb_sampler((const void *)ei_sensor_imu_read_data(N_SENSOR_AXES), SIZEOF_SENSOR_VALUES_IN_SAMPLE);
}

bool ei_accel_sample_start(sampler_callback callsampler, float sample_interval_ms)
{
    EiDeviceInfo *dev = EiDeviceInfo::get_device();
    inertial_cb_sampler = callsampler;

    dev->set_sample_interval_ms(sample_interval_ms);

    dev->set_state(eiStateSampling);

    dev->start_sample_thread(&ei_accel_read_data, sample_interval_ms);

    return true;
}
