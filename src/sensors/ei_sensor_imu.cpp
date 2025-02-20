/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
