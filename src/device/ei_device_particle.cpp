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
#include <string>

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "firmware-sdk/ei_device_info_lib.h"
#include "firmware-sdk/ei_device_memory.h"
#include "ei_microphone.h"
#include "ei_device_particle.h"
#include "ei_flash_memory.h"

#include "Particle.h"

/* Private variables ------------------------------------------------------- */
static void (*sample_read_function)(void) = NULL;
static void timer_callback(void);
static Timer timer(1000, &timer_callback);

/** Sensors */
typedef enum
{
    MICROPHONE = 0,

}used_sensors_t;

/**
 * Timer callback for grabbing sample data
*/
static void timer_callback(void)
{
    if(sample_read_function) {
        sample_read_function();
    }
}


/******
 *
 * @brief EdgeImpulse Device structure and information
 *
 ******/


EiDeviceParticle::EiDeviceParticle(EiDeviceMemory* mem)
{
    EiDeviceInfo::memory = mem;

    init_device_id();

    load_config();

    device_type = "PARTICLE_P2"; /* TODO-MODIFY update the device name */

}

EiDeviceParticle::~EiDeviceParticle()
{

}

EiDeviceInfo* EiDeviceInfo::get_device(void)
{
    /* Initializing EdgeImpulse classes here in order for
     * Flash memory to be initialized before mainloop start
     */
    static EiFlashMemory memory(sizeof(EiConfig));
    static EiDeviceParticle dev(static_cast<EiDeviceMemory*>(&memory));

    return &dev;
}

void EiDeviceParticle::init_device_id(void)
{
    char temp[18];

    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(temp, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    device_id = std::string(temp);
}

void EiDeviceParticle::clear_config(void)
{
    EiDeviceInfo::clear_config();

    init_device_id();
    save_config();
}

uint32_t EiDeviceParticle::get_data_output_baudrate(void)
{
    return EI_DEVICE_BAUDRATE;
}

/**
 * @brief      Set output baudrate to max
 *
 */
void EiDeviceParticle::set_max_data_output_baudrate()
{

}

/**
 * @brief      Set output baudrate to default
 *
 */
void EiDeviceParticle::set_default_data_output_baudrate()
{
    /* TODO-ADD use the EI_DEVICE_BAUDRATE define with target specific code */
}

bool EiDeviceParticle::start_sample_thread(void (*sample_read_cb)(void), float sample_interval_ms)
{
    sample_read_function = sample_read_cb;

    timer.changePeriod((int)sample_interval_ms);

    return timer.start();
}

bool EiDeviceParticle::stop_sample_thread(void)
{
    timer.stop();

    this->set_state(eiStateIdle);

    return true;
}

void EiDeviceParticle::set_state(EiState state)
{
    this->state = state;

    /* TODO-OPTIONAL Use when LED GPIO is implemented */
    switch(state) {
        case eiStateErasingFlash:
        case eiStateSampling:
        case eiStateUploading:
        case eiStateFinished:
        case eiStateIdle:
        default:
            break;
    }
}

EiState EiDeviceParticle::get_state(void)
{
    return this->state;
}

EiSnapshotProperties EiDeviceParticle::get_snapshot_list(void)
{
    ei_device_snapshot_resolutions_t *res = NULL;
    uint8_t res_num = 0;

    EiSnapshotProperties props = {
        .has_snapshot = false,
        .support_stream = false,
        .color_depth = "",
        .resolutions_num = 0,
        .resolutions = res
    };

    // if(this->cam->is_camera_present() == true) {
    //     this->cam->get_resolutions(&res, &res_num);
    //     props.has_snapshot = true;
    //     props.support_stream = false; /* TODO-OPTIONAL change to true for live preview of image frames in Studio */
    //     props.color_depth = "RGB";
    //     props.resolutions_num = res_num;
    //     props.resolutions = res;
    // }

    return props;
}

/**
 * @brief      Create sensor list with sensor specs
 *             The studio and daemon require this list
 * @param      sensor_list       Place pointer to sensor list
 * @param      sensor_list_size  Write number of sensors here
 *
 * @return     False if all went ok
 */
bool EiDeviceParticle::get_sensor_list(const ei_device_sensor_t **sensor_list, size_t *sensor_list_size)
{
    /* Calculate number of bytes available on flash for sampling, reserve 1 block for header + overhead */
    uint32_t available_bytes = (memory->get_available_sample_blocks()-1) * memory->block_size;

    sensors[MICROPHONE].name = "Microphone";
    sensors[MICROPHONE].start_sampling_cb = &ei_microphone_sample_start;
    sensors[MICROPHONE].max_sample_length_s = available_bytes / (16000 * 2);
    sensors[MICROPHONE].frequencies[0] = 16000.0f;
    sensors[MICROPHONE].frequencies[1] = 8000.0f;
    sensors[MICROPHONE].frequencies[2] = 4000.0f;
    *sensor_list      = sensors;
    *sensor_list_size = EI_DEVICE_N_SENSORS;

    return false;
}
