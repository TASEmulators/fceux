/* FCE Ultra - NES/Famicom Emulator - Emscripten audio
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 Valtteri "tsone" Heikkila
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "em.h"
#include "../../utils/memory.h"
#include "../../fceu.h"
#include <emscripten.h>

// DEBUG: Define a test to output a sine tone.
#define TEST_SINE_AT_FILL   0
#define TEST_SINE_AT_WRITE  0

extern int EmulationPaused; // defined in fceu.cpp

int em_audio_rate = 44100;
double em_audio_frame_samples = em_audio_rate / NTSC_FPS;

static float *s_buffer = 0;
static int s_buffer_read = 0;
static int s_buffer_write = 0;
static int s_buffer_count = 0;

static bool s_muted = false;

#if TEST_SINE_AT_FILL || TEST_SINE_AT_WRITE
#include <math.h>
static double test_sine_phase = 0.0;
#endif

// Consumes samples from audio buffer. Modifies the buffer indexes and
// returns the number of available samples. Does NOT modify HW audio buffer.
static int ConsumeBuffer(int samples)
{
    const int available = (s_buffer_count > samples) ? samples : s_buffer_count;
    s_buffer_read = (s_buffer_read + available) & AUDIO_BUF_MASK;
    s_buffer_count -= available;
    return available;
}

// Copy buffer samples to HW buffer. Fill zeros if there's not enough samples.
static void CopyBuffer()
{
    int buffer_read = s_buffer_read; // ConsumeBuffer() modifies this.

    // FIXME: Hard-coded AUDIO_HW_BUF_MAX.
    int available = ConsumeBuffer(AUDIO_HW_BUF_MAX);
    EM_ASM({
        const s_buffer = $0;
        const s_buffer_read = $1;
        let available = $2;

        const channelData = Module.currentOutputBuffer.getChannelData(0);

        // FIXME: Hard-coded AUDIO_BUF_MAX.
        let m = 8192 - s_buffer_read;
        if (m > available) {
            m = available;
        }

#if 1
        // Float32Array.set() version.
        // FIXME: Hard-coded AUDIO_HW_BUF_MAX.
        let samples = 2048 - available + 1;
        available = available - m;

        if (m > 0) {
            const j = s_buffer + s_buffer_read;
            channelData.set(HEAPF32.subarray(j, j + m));
        }
        if (available > 0) {
            channelData.set(HEAPF32.subarray(s_buffer, s_buffer + available), m);
        }
        m += available - 1;
        while (--samples) {
            channelData[++m] = 0;
        }

#else
        // Regular version without Float32Array.set().
        let j = s_buffer + s_buffer_read - 1;
        // FIXME: Hard-coded AUDIO_HW_BUF_MAX.
        let samples = 2048 - available + 1;
        available = available - m + 1;
        ++m;

        let i = -1;
        while (--m) {
            channelData[++i] = HEAPF32[++j];
        }
        j = s_buffer - 1;
        while (--available) {
            channelData[++i] = HEAPF32[++j];
        }
        while (--samples) {
            channelData[++i] = 0;
        }
#endif
    }, (ptrdiff_t) s_buffer >> 2, buffer_read, available);
}

// Fill HW audio buffer with silence.
static void SilenceBuffer()
{
    EM_ASM({
        const channelData = Module.currentOutputBuffer.getChannelData(0);
        // FIXME: Hard-coded AUDIO_HW_BUF_MAX.
        for (let i = 2048 - 1; i >= 0; --i) {
            channelData[i] = 0;
        }
    });
}

// Callback for filling HW audio buffer, exposed to js.
extern "C" void audioBufferCallback()
{
    if (!EmulationPaused && !s_muted) {
        CopyBuffer();
    } else {
        SilenceBuffer();
    }

    if (s_muted) {
        // FIXME: Hard-coded AUDIO_HW_BUF_MAX.
        ConsumeBuffer(AUDIO_HW_BUF_MAX);
    }
}

static void SetSampleRate(int sample_rate)
{
    em_audio_rate = sample_rate;
    em_audio_frame_samples = em_audio_rate / (FSettings.PAL ? PAL_FPS : NTSC_FPS);
    FCEUI_Sound(em_audio_rate);
}

// Returns the sample_rate (Hz) of initialized audio context.
static int ContextInit()
{
    return EM_ASM_INT({
        const context = Module._audioContext;
        const source = context.createBufferSource();

        // Always mono (1 channel).
        const processor = context.createScriptProcessor($0, 0, 1);
        Module.scriptProcessorNode = processor;
        source.connect(processor); // For iOS/iPadOS 13 Safari, processor requires a source.
        processor.onaudioprocess = function(ev) {
            Module.currentOutputBuffer = ev.outputBuffer;
            Module._audioBufferCallback();
        };
        processor.connect(context.destination);

        return context.sampleRate;
    }, AUDIO_HW_BUF_MAX);
}

bool Audio_IsInitialized()
{
    static bool initialized = false;

    if (initialized) {
        return true;
    }

    if (EM_ASM_INT({ return !Module.hasOwnProperty('_audioContext'); })) {
        return false;
    }

    SetSampleRate(ContextInit());
    FCEUD_Message("Web Audio context initialized.");
    initialized = true;
    return initialized;
}

bool Audio_Init()
{
    if (s_buffer) {
        return true;
    }

    s_buffer = (float*) FCEU_dmalloc(sizeof(float) * AUDIO_BUF_MAX);
    if (!s_buffer) {
        FCEUD_PrintError("Failed to create audio buffer.");
        return false;
    }
    s_buffer_read = s_buffer_write = s_buffer_count = 0;

    FCEUI_SetSoundVolume(150); // Maximum volume.
    FCEUI_SetSoundQuality(0); // Low quality.
    SetSampleRate(44100);
    FCEUI_SetTriangleVolume(256);
    FCEUI_SetSquare1Volume(256);
    FCEUI_SetSquare2Volume(256);
    FCEUI_SetNoiseVolume(256);
    FCEUI_SetPCMVolume(256);
    return true;
}

int Audio_GetBufferCount()
{
    return s_buffer_count;
}

void Audio_Write(int32 *buf, int count)
{
    if (EmulationPaused || (count <= 0)) {
        return;
    }

    int free_count = AUDIO_BUF_MAX - s_buffer_count;
    if (count > free_count) {
        // Tries to write <count> samples but buffer has only <free_count>. Cap it.
        count = free_count;
    }

#if TEST_SINE_AT_WRITE

    // Sine wave test outputs a 440Hz tone.
    s_buffer_count += count;
    ++count;
    while (--count) {
        s_buffer[s_buffer_write] = 0.5 * sin(test_sine_phase * (2.0*M_PI * 440.0/em_audio_rate));
        s_buffer_write = (s_buffer_write + 1) & AUDIO_BUF_MASK;
        ++test_sine_phase;
    }

#else

#if 1 // Optimized version of the #if branch below.

    int i = s_buffer_write - 1;
    int j = -1;
    int m = AUDIO_BUF_MAX - s_buffer_write;
    if (m > count) {
        m = count;
    }

    s_buffer_count += count;
    s_buffer_write = (s_buffer_write + count) & AUDIO_BUF_MASK;

    count = count - m + 1;
    ++m;

    while (--m) {
        s_buffer[++i] = buf[++j] * (1 / 32768.0);
    }
    i = -1;
    while (--count) {
        s_buffer[++i] = buf[++j] * (1 / 32768.0);
    }

#else

    for (int i = 0; i < count; ++i) {
        s_buffer[s_buffer_write] = buf[i];
        s_buffer_write = (s_buffer_write + 1) & AUDIO_BUF_MASK;
    }

    s_buffer_count += count;

#endif

#endif // TEST_SINE_AT_WRITE
}

void Audio_SetMuted(bool muted)
{
    s_muted = muted;
}

bool Audio_Muted()
{
    return s_muted;
}
