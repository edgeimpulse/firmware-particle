/*
 * Copyright (c) 2023 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
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
