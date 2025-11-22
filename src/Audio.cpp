#include "Audio.h"

Audio::Audio() {
    reset();
}

bool Audio::init(Config* config, lrcpp::Logger* logger) {
    reset();

    _logger = logger;

    _logger->info("Audio subsystem initialized");

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

const std::vector<int16_t>& Audio::getSamples() const {
    return _samples;
}

void Audio::clearSamples() {
    _samples.clear();
}

bool Audio::setSystemAvInfo(retro_system_av_info const* info) {
     _logger->info("Audio:setSystemAvInfo  sample_rate=%f", info->timing.sample_rate);
    _coreSampleRate = info->timing.sample_rate;
    return true;
}


bool Audio::setAudioCallback(retro_audio_callback const* callback) {
    _logger->warn("RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK not implemented");
    return true;
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
