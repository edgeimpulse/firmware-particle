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

#ifndef EI_SENSOR_FUSION_TEMPLATE_H
#define EI_SENSOR_FUSION_TEMPLATE_H

/* Include ----------------------------------------------------------------- */
#include "firmware-sdk/ei_fusion.h"

/** Number of axis used and sample data format */
#define N_SENSOR_AXES          3
#define SIZEOF_SENSOR_VALUES_IN_SAMPLE   (sizeof(float) * N_SENSOR_AXES)

/* Function prototypes ----------------------------------------------------- */
bool ei_sensor_imu_init(void);
float *ei_sensor_imu_read_data(int n_samples);
bool ei_accel_sample_start(sampler_callback callsampler, float sample_interval_ms);

/* TODO-MODIFY Update with the sensor specific characteristics */
static const ei_device_fusion_sensor_t imu_sensor = {
    // name of sensor module to be displayed in fusion list
    "Accelerometer",
    // number of sensor module axis
    N_SENSOR_AXES,
    // sampling frequencies
    { 20.f, 62.5f, 100.f},
    // axis name and units payload (must be same order as read in)
    { { "accX ", "m/s2" }, { "accY ", "m/s2" }, { "accZ ", "m/s2" } },
    // reference to read data function
    &ei_sensor_imu_read_data,
    0
};

#endif /* EI_SENSOR_FUSION_TEMPLATE_H */
