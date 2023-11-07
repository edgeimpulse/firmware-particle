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
