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
#ifndef EI_FLASH_MEMORY_H
#define EI_FLASH_MEMORY_H

/* Includes ---------------------------------------------------------------- */
#include "firmware-sdk/ei_device_memory.h"

/*
  Flash Related Parameter Define
*/
#define RAM_ERASE_TIME      10
#define RAM_N_BLOCKS        112
#define RAM_BLOCK_SIZE      512

class EiFlashMemory : public EiDeviceMemory {
protected:
    uint32_t read_data(uint8_t *data, uint32_t address, uint32_t num_bytes);
    uint32_t write_data(const uint8_t *data, uint32_t address, uint32_t num_bytes);
    uint32_t erase_data(uint32_t address, uint32_t num_bytes);
    uint32_t get_available_sample_blocks(void) override;
    uint32_t get_available_sample_bytes(void) override;
    bool save_config(const uint8_t *config, uint32_t config_size) override;
    bool load_config(uint8_t *config, uint32_t config_size) override;
    uint32_t read_sample_data(uint8_t *sample_data, uint32_t address, uint32_t sample_data_size) override;
    uint32_t write_sample_data(const uint8_t *sample_data, uint32_t address, uint32_t sample_data_size) override;
    uint32_t erase_sample_data(uint32_t address, uint32_t num_bytes) override;

public:
    EiFlashMemory(uint32_t config_size);
};

#endif /* EI_FLASH_MEMORY_H */
