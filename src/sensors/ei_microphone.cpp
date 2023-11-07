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
#include "ei_microphone.h"
#include "ei_device_particle.h"

#include "misc/sensor_aq_mbedtls/sensor_aq_mbedtls_hs256.h"
#include "firmware-sdk/sensor-aq/sensor_aq.h"
#include "edge-impulse-sdk/CMSIS/DSP/Include/arm_math.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"

#include "Particle.h"
#include "Microphone_PDM.h"


typedef struct {
    int16_t *buffers[2];
    uint8_t buf_select;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

/* Extern declared --------------------------------------------------------- */

using namespace ei;

static size_t ei_write(const void*, size_t size, size_t count, EI_SENSOR_AQ_STREAM*)
{
    ei_printf("Writing: %d\r\n", count);
    return count;
}

static int ei_seek(EI_SENSOR_AQ_STREAM*, long int offset, int origin)
{
    ei_printf("Seeking: %d\r\n", offset);
    return 0;
}

/* Private variables ------------------------------------------------------- */
static bool record_ready = false;
static uint32_t headerOffset;
static uint32_t samples_required;
static uint32_t current_sample;
static uint32_t audio_sampling_frequency = 16000;

static void (*dma_callback_func)(uint32_t n_bytes) = NULL;
static int16_t *dma_copy_buf = NULL;

static inference_t inference;

static unsigned char ei_mic_ctx_buffer[1024];
static sensor_aq_signing_ctx_t ei_mic_signing_ctx;
static sensor_aq_mbedtls_hs256_ctx_t ei_mic_hs_ctx;
static sensor_aq_ctx ei_mic_ctx = {
    { ei_mic_ctx_buffer, 1024 },
    &ei_mic_signing_ctx,
    &ei_write,
    &ei_seek,
    NULL,
};

/* Private functions ------------------------------------------------------- */


int ei_microphone_init(void)
{
    int err = Microphone_PDM::instance()
    .withOutputSize(Microphone_PDM::OutputSize::SIGNED_16)
    .withRange(Microphone_PDM::Range::RANGE_32768)
    .withSampleRate(audio_sampling_frequency)
    .init();

    return err;
}

static void audio_buffer_callback(uint32_t n_bytes)
{
    EiDeviceMemory *mem = EiDeviceInfo::get_device()->get_memory();
    mem->write_sample_data((const uint8_t*)dma_copy_buf, headerOffset + current_sample, n_bytes);


    ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, (uint8_t*)dma_copy_buf, n_bytes);

    current_sample += n_bytes;
    if(current_sample >= (samples_required << 1)) {
        record_ready = false;
    }
}

static void audio_buffer_inference_callback(uint32_t n_bytes)
{
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = *(dma_copy_buf + i);

        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void pdm_data_ready_callback(void)
{
    int n_dma_samples;

    if(Microphone_PDM::instance().samplesAvailable()) {
        n_dma_samples = Microphone_PDM::instance().getNumberOfSamples();

        Microphone_PDM::instance().copySamples((void *)dma_copy_buf);

        if(n_dma_samples) {

            for(int i = 0; i < n_dma_samples; i++) {
                *(dma_copy_buf + i) <<= 2;
            }

            dma_callback_func(n_dma_samples * 2);
        }
    }
}



static void finish_and_upload(void) {

    ei_printf("Done sampling, total bytes collected: %u\n", current_sample);

    ei_printf("[1/1] Uploading file to Edge Impulse...\n");
    ei_printf("Not uploading file, not connected to WiFi. Used buffer, from=%lu, to=%lu.\n", 0, current_sample + headerOffset);
    ei_printf("[1/1] Uploading file to Edge Impulse OK (took 0 ms.)\n");

    ei_printf("OK\n");
}

static int insert_ref(char *buffer, int hdrLength)
{
    #define EXTRA_BYTES(a)  ((a & 0x3) ? 4 - (a & 0x3) : (a & 0x03))
    const char *ref = "Ref-BINARY-i16";
    int addLength = 0;
    int padding = EXTRA_BYTES(hdrLength);

    buffer[addLength++] = 0x60 + 14 + padding;
    for(size_t i = 0; i < strlen(ref); i++) {
        buffer[addLength++] = *(ref + i);
    }
    for(int i = 0; i < padding; i++) {
        buffer[addLength++] = ' ';
    }

    buffer[addLength++] = 0xFF;

    return addLength;
}

static bool create_header(void)
{
    EiDeviceInfo *dev = EiDeviceInfo::get_device();
    EiDeviceMemory *mem = dev->get_memory();
    sensor_aq_init_mbedtls_hs256_context(&ei_mic_signing_ctx, &ei_mic_hs_ctx, dev->get_sample_hmac_key().c_str());

    sensor_aq_payload_info payload = {
        dev->get_id_pointer(),
        dev->get_type_pointer(),
        1000.0f / static_cast<float>(audio_sampling_frequency),
        { { "audio", "wav" } }
    };

    int tr = sensor_aq_init(&ei_mic_ctx, &payload, NULL, true);

    if (tr != AQ_OK) {
        ei_printf("sensor_aq_init failed (%d)\n", tr);
        return false;
    }

    // then we're gonna find the last byte that is not 0x00 in the CBOR buffer.
    // That should give us the whole header
    size_t end_of_header_ix = 0;
    for (size_t ix = ei_mic_ctx.cbor_buffer.len - 1; ix >= 0; ix--) {
        if (((uint8_t*)ei_mic_ctx.cbor_buffer.ptr)[ix] != 0x0) {
            end_of_header_ix = ix;
            break;
        }
    }

    if (end_of_header_ix == 0) {
        ei_printf("Failed to find end of header\n");
        return false;
    }


    int ref_size = insert_ref(((char*)ei_mic_ctx.cbor_buffer.ptr + end_of_header_ix), end_of_header_ix);

    // and update the signature
    tr = ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, (uint8_t*)(ei_mic_ctx.cbor_buffer.ptr + end_of_header_ix), ref_size);
    if (tr != 0) {
        ei_printf("Failed to update signature from header (%d)\n", tr);
        return false;
    }

    end_of_header_ix += ref_size;

    // Write to blockdevice
    tr = mem->write_sample_data((const uint8_t*)ei_mic_ctx.cbor_buffer.ptr, 0, end_of_header_ix);

    if (tr != (int)end_of_header_ix) {
        ei_printf("Failed to write to header blockdevice\n");
        return false;
    }

    headerOffset = end_of_header_ix;

    return true;
}


/* Public functions -------------------------------------------------------- */

bool ei_microphone_record(uint32_t sample_length_ms, uint32_t start_delay_ms, bool print_start_messages)
{
    EiDeviceMemory *mem = EiDeviceInfo::get_device()->get_memory();

    if (print_start_messages) {
        ei_printf("Starting in %u ms... (or until all flash was erased)\n", start_delay_ms < 2000 ? 2000 : start_delay_ms);
    }

    if(start_delay_ms < 2000) {
        ei_sleep(2000 - start_delay_ms);
    }

    if(mem->erase_sample_data(0, (samples_required << 1) + 4096) != (samples_required << 1) + 4096) {
        return false;
    }

    create_header();

    if (print_start_messages) {
        ei_printf("Sampling...\n");
    }

    return true;
}

bool ei_microphone_inference_start(uint32_t n_samples, float interval_ms)
{
    EiDeviceInfo *dev = EiDeviceInfo::get_device();

    inference.buffers[0] = (int16_t *)ei_malloc(n_samples * sizeof(int16_t));

    if(inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (int16_t *)ei_malloc(n_samples * sizeof(int16_t));

    if(inference.buffers[1] == NULL) {
        ei_free(inference.buffers[0]);
        return false;
    }

    // Max DMA buf size on Realtek
    dma_copy_buf = (int16_t *)ei_malloc(256 * 2);

    if(dma_copy_buf == NULL) {
        ei_free(inference.buffers[0]);
        ei_free(inference.buffers[1]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;


    /* Calclate sample rate from sample interval */
    audio_sampling_frequency = (uint32_t)(1000.f / interval_ms);

    if(audio_sampling_frequency != 16000) {
        ei_printf("ERR: Audio sampling supports only 16000Hz\r\n");
        return false;
    }

    /* Set callback function */
    dma_callback_func = &audio_buffer_inference_callback;

    int err = Microphone_PDM::instance().start();

	if (err) {
		ei_printf("ERR: Audio init error %d", err);
        ei_microphone_inference_end();
        return false;
	}

     /* Empty DMA buffers */
    while(Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples){})){};

    dev->start_sample_thread(&pdm_data_ready_callback, 2);

    return true;
}

/**
 * @brief      Wait for a full buffer
 *
 * @return     In case of an buffer overrun return false
 */
bool ei_microphone_inference_record(bool first_run)
{
    bool ret = false;

    if (inference.buf_ready == 1 && first_run == true) {
        ei_printf(
            "Error sample buffer overrun. Decrease the number of slices per model window "
            "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
    }

    if(inference.buf_ready) {
        ret = true;
        inference.buf_ready = 0;
    }

    return ret;
}

/**
 * @brief      Reset buffer counters for non-continuous inferecing
 */
void ei_microphone_inference_reset_buffers(void)
{
    /* Empty DMA buffers */
    while(Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples){})){};

    inference.buf_ready = 0;
    inference.buf_count = 0;
}

/**
 * Get raw audio signal data
 */
int ei_microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    return numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);
}


bool ei_microphone_inference_end(void)
{
    EiDeviceInfo *dev = EiDeviceInfo::get_device();

    dev->stop_sample_thread();

    Microphone_PDM::instance().stop();
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
    ei_free(dma_copy_buf);

    return true;
}

/**
 * Sample raw data
 */
bool ei_microphone_sample_start(void)
{
    EiDeviceInfo *dev = EiDeviceInfo::get_device();
    EiDeviceMemory *mem = dev->get_memory();
    // this sensor does not have settable interval...
    // ei_config_set_sample_interval(static_cast<float>(1000) / static_cast<float>(audio_sampling_frequency));

    ei_printf("Sampling settings:\n");
    ei_printf("\tInterval: %.5f ms.\n", dev->get_sample_interval_ms());
    ei_printf("\tLength: %lu ms.\n", dev->get_sample_length_ms());
    ei_printf("\tName: %s\n", dev->get_sample_label().c_str());
    ei_printf("\tHMAC Key: %s\n", dev->get_sample_hmac_key().c_str());
    ei_printf("\tFile name: /fs/%s\n", dev->get_sample_label().c_str());

    samples_required = (uint32_t)((dev->get_sample_length_ms()) / dev->get_sample_interval_ms());

    /* Round to even number of samples for word align flash write */
    if(samples_required & 1) {
        samples_required++;
    }

    current_sample = 0;

    /* Calclate sample rate from sample interval */
    audio_sampling_frequency = (uint32_t)(1000.f / dev->get_sample_interval_ms());

    if(audio_sampling_frequency != 16000) {
        ei_printf("ERR: Audio sampling supports only 16000Hz\r\n");
        return false;
    }

    /* Set callback function */
    dma_callback_func = &audio_buffer_callback;

    // Max DMA buf size on Realtek
    dma_copy_buf = (int16_t *)ei_malloc(256 * 2);

    if(dma_copy_buf == NULL) {
        ei_printf("ERR: memory allocation failed\r\n");
        return false;
    }

	int err = Microphone_PDM::instance().start();

	if (err) {
		ei_printf("ERR: pdm driver error %d", err);
        return false;
	}

    bool r = ei_microphone_record(dev->get_sample_length_ms(), (((samples_required <<1)/ mem->block_size) * mem->block_erase_time), true);
    if (!r) {
        return r;
    }
    record_ready = true;

     /* Empty DMA buffers */
    while(Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples){})){};

    while(record_ready == true) {
        pdm_data_ready_callback();
    };

    ei_free((void *)dma_copy_buf);
    dma_copy_buf = NULL;

    int ctx_err = ei_mic_ctx.signature_ctx->finish(ei_mic_ctx.signature_ctx, ei_mic_ctx.hash_buffer.buffer);
    if (ctx_err != 0) {
        ei_printf("Failed to finish signature (%d)\n", ctx_err);
        return false;
    }

    // load the first page in flash...
    uint8_t *page_buffer = (uint8_t*)ei_malloc(mem->block_size);
    if (!page_buffer) {
        ei_printf("Failed to allocate a page buffer to write the hash\n");
        return false;
    }

    uint32_t j = mem->read_sample_data(page_buffer, 0, mem->block_size);
    if (j != mem->block_size) {
        ei_printf("Failed to read first page\n");
        free(page_buffer);
        return false;
    }

    // update the hash
    uint8_t *hash = ei_mic_ctx.hash_buffer.buffer;
    // we have allocated twice as much for this data (because we also want to be able to represent in hex)
    // thus only loop over the first half of the bytes as the signature_ctx has written to those
    for (size_t hash_ix = 0; hash_ix < ei_mic_ctx.hash_buffer.size / 2; hash_ix++) {
        // this might seem convoluted, but snprintf() with %02x is not always supported e.g. by newlib-nano
        // we encode as hex... first ASCII char encodes top 4 bytes
        uint8_t first = (hash[hash_ix] >> 4) & 0xf;
        // second encodes lower 4 bytes
        uint8_t second = hash[hash_ix] & 0xf;

        // if 0..9 -> '0' (48) + value, if >10, then use 'a' (97) - 10 + value
        char first_c = first >= 10 ? 87 + first : 48 + first;
        char second_c = second >= 10 ? 87 + second : 48 + second;

        page_buffer[ei_mic_ctx.signature_index + (hash_ix * 2) + 0] = first_c;
        page_buffer[ei_mic_ctx.signature_index + (hash_ix * 2) + 1] = second_c;
    }

    j = mem->erase_sample_data(0, mem->block_size);
    if (j != mem->block_size) {
        ei_printf("Failed to erase first page\n");
        free(page_buffer);
        return false;
    }

    j = mem->write_sample_data(page_buffer, 0, mem->block_size);

    ei_free(page_buffer);

    if (j != mem->block_size) {
        ei_printf("Failed to write first page with updated hash\n");
        return false;
    }


    finish_and_upload();

    return true;
}
