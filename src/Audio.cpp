#include "Audio.h"

Audio::Audio() {
    reset();
}

bool Audio::init(Config* config, lrcpp::Logger* logger) {
    reset();

    _logger = logger;

    _logger->info("Audio subsystem initialized");

    char const* driverName = nullptr;

    if (config->getOption("sdl2lrcpp_audio_device", &driverName)) {
        _deviceName = driverName;
    }

    return true;
}

void Audio::destroy() {
    reset();
}

double Audio::getCoreSampleRate() const {
    return _coreSampleRate;
}

void Audio::clear() {
    _samples.clear();
}

void Audio::present() {
    // _logger->debug("Audio Not implemented");
}

bool Audio::setSystemAvInfo(retro_system_av_info const* info) {
    return true;
}

bool Audio::setAudioCallback(retro_audio_callback const* callback) {
    (void)callback;
    _logger->warn("RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK not implemented");
    return false;
}

size_t Audio::sampleBatch(int16_t const* data, size_t frames) {
    size_t const size = _samples.size();
    _samples.resize(size + frames * 2);
    memcpy(_samples.data() + size, data, frames * 4);

    _logger->debug("%zu audio frames queued", frames);
    return frames;
}

void Audio::sample(int16_t left, int16_t right) {
    int16_t frame[2] = {left, right};
    sampleBatch(frame, 1);
}

void Audio::reset() {
    _logger = nullptr;

    _deviceName.clear();
    _coreSampleRate = 0.0;
    // _audioDev = 0;

    _samples.clear();
}
