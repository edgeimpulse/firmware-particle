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

/**
 * RAM Implementation using a separate derived class from EiDeviceMemory.
 * The virtual functions used with the EiDeviceRAM class hardfault on Particle Photon 2
*/

/* Includes ---------------------------------------------------------------- */
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "ei_flash_memory.h"

/* Private variables ------------------------------------------------------- */
uint8_t ram_memory[RAM_N_BLOCKS * RAM_BLOCK_SIZE];

uint32_t EiFlashMemory::read_data(uint8_t *data, uint32_t address, uint32_t num_bytes)
{
    if (num_bytes > memory_size - address) {
        num_bytes = memory_size - address;
    }

    memcpy(data, &ram_memory[address], num_bytes);

    return num_bytes;
}

uint32_t EiFlashMemory::write_data(const uint8_t *data, uint32_t address, uint32_t num_bytes)
{
    if (num_bytes > memory_size - address) {
        num_bytes = memory_size - address;
    }

    memcpy(&ram_memory[address], data, num_bytes);

    return num_bytes;
}

uint32_t EiFlashMemory::erase_data(uint32_t address, uint32_t num_bytes)
{
    if (num_bytes > memory_size - address) {
        num_bytes = memory_size - address;
    }

    memset(&ram_memory[address], 0, num_bytes);

    return num_bytes;
}

uint32_t EiFlashMemory::get_available_sample_blocks(void)
{
    return memory_blocks - used_blocks;
}

uint32_t EiFlashMemory::get_available_sample_bytes(void)
{
    return (memory_blocks - used_blocks) * block_size;
}

bool EiFlashMemory::save_config(const uint8_t *config, uint32_t config_size)
{
    uint32_t used_bytes = used_blocks * block_size;
    if (erase_data(0, used_bytes) != used_bytes) {
        return false;
    }

    if (write_data(config, 0, config_size) != config_size) {
        return false;
    }

    return true;
}

bool EiFlashMemory::load_config(uint8_t *config, uint32_t config_size)
{
    if (read_data(config, 0, config_size) != config_size) {
        return false;
    }
    return true;
}

uint32_t EiFlashMemory::read_sample_data(uint8_t *sample_data, uint32_t address, uint32_t sample_data_size)
{
    uint32_t offset = used_blocks * block_size;

    return read_data(sample_data, offset + address, sample_data_size);
}

uint32_t EiFlashMemory::write_sample_data(const uint8_t *sample_data, uint32_t address, uint32_t sample_data_size)
{
    uint32_t offset = used_blocks * block_size;

    return write_data(sample_data, offset + address, sample_data_size);
}

uint32_t EiFlashMemory::erase_sample_data(uint32_t address, uint32_t num_bytes)
{
    uint32_t offset = used_blocks * block_size;

    return erase_data(offset + address, num_bytes);
}


EiFlashMemory::EiFlashMemory(uint32_t config_size):
    EiDeviceMemory(config_size, RAM_ERASE_TIME, RAM_N_BLOCKS * RAM_BLOCK_SIZE, RAM_BLOCK_SIZE)
{

}