#include <alsa/asoundlib.h>
#include <assert.h>
#include <stdint.h>

// https://github.com/runemopar/wav.h
#include "../wav.h/wav.h"

#define ALSA_TRY(to_try)                                              \
    {                                                                 \
        int err;                                                      \
        if ((err = to_try) < 0) {                                     \
            fprintf(stderr, "Error: (%s) in %s\n", snd_strerror(err), \
                    #to_try);                                         \
            exit(1);                                                  \
        }                                                             \
    }

void set_hw_params(snd_pcm_t *playback_handle, snd_pcm_access_t pcm_access_type,
                   snd_pcm_format_t pcm_format_type, unsigned int sample_rate,
                   unsigned int num_channels) {
    // harwdare parameters structure
    snd_pcm_hw_params_t *hardware_params;
    // allocate hardware parameter structure
    ALSA_TRY(snd_pcm_hw_params_malloc(&hardware_params));
    // Initialize hardware parameter structure
    ALSA_TRY(snd_pcm_hw_params_any(playback_handle, hardware_params));

    // Set parameters
    ALSA_TRY(snd_pcm_hw_params_set_access(playback_handle, hardware_params,
                                          pcm_access_type));
    ALSA_TRY(snd_pcm_hw_params_set_format(playback_handle, hardware_params,
                                          pcm_format_type));
    ALSA_TRY(snd_pcm_hw_params_set_rate(playback_handle, hardware_params,
                                        sample_rate, 0));
    ALSA_TRY(snd_pcm_hw_params_set_channels(playback_handle, hardware_params,
                                            num_channels));
    ALSA_TRY(snd_pcm_hw_params(playback_handle, hardware_params));

    // Free hardware_params
    snd_pcm_hw_params_free(hardware_params);
}

#define AUDIO_FILE "test_audio/schreibmaschine.wav"
#define AUDIO_DEVICE "default"
#define ACCESS_TYPE SND_PCM_ACCESS_RW_INTERLEAVED
#define FORMAT_TYPE SND_PCM_FORMAT_S16_LE
#define BUF_SIZE 256

int main() {
    FILE *file = fopen(AUDIO_FILE, "r");

    assert(file != NULL && "Can't open file for reading");

    wav_fmt_desc fmt_desc;
    void *wav_data = wav_load_data(file, &fmt_desc);

    assert(wav_data != NULL && "Failed to read wave data from file");

    assert(fmt_desc.wav_format == WAVE_FORMAT_PCM);

    snd_pcm_t *playback_handle;

    // attempt to open audio device
    ALSA_TRY(snd_pcm_open(&playback_handle, AUDIO_DEVICE,
                          SND_PCM_STREAM_PLAYBACK, 0));

    set_hw_params(playback_handle, ACCESS_TYPE, FORMAT_TYPE,
                  fmt_desc.sample_rate, fmt_desc.num_channels);

    ALSA_TRY(snd_pcm_prepare(playback_handle));

    char buf[BUF_SIZE];
    size_t frames_left = fmt_desc.num_samples;
    size_t frame_size = fmt_desc.num_channels * fmt_desc.bytes_per_sample;
    char *playhead = (char *)wav_data;

    size_t frames_in_a_buf = BUF_SIZE / frame_size;

    while (1) {
        size_t nframes =
            frames_in_a_buf < frames_left ? frames_in_a_buf : frames_left;

        size_t nbytes = nframes * frame_size;

        if (nbytes <= 0) break;

        memcpy(buf, playhead, nbytes);
        playhead += nbytes;
        frames_left -= nframes;

        // write interleaved samples
        if (snd_pcm_writei(playback_handle, buf, nframes) != nframes) {
            fprintf(stderr, "write to audio interface failed\n");
            snd_pcm_prepare(playback_handle);
        }
    }

    snd_pcm_drain(playback_handle);
    snd_pcm_close(playback_handle);
}
