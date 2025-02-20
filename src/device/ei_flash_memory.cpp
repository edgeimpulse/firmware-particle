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