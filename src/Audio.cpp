#include "Audio.hpp"

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
    // Push queued samples to audio playback if available
    if (_audio_player == nullptr) {
        return;
    }

    _audio_player->present(&_samples);

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

void Audio::set_audio_player(godot::Node* player) {
    AudioPlayerFactory factory(_logger);
    _audio_player = factory.createAudioPlayer(player, _coreSampleRate);

    if (_audio_player == nullptr) {
        _logger->error("Audio Player is null, audio output disabled.");
    }
 
}

size_t Audio::sampleBatch(int16_t const* data, size_t frames) {
    if (frames != 0) {
        
        _samples.resize((int)frames);

        for (size_t i = 0; i < frames; ++i) {
            float left = (float)data[i * 2] / 32768.0f;
            float right = (float)data[i * 2 + 1] / 32768.0f;
            godot::Vector2 v(left, right);
            _samples[(int)i] = v;
        }
    }

    return frames;
}

void Audio::sample(int16_t left, int16_t right) {
    int16_t frame[2] = {left, right};
    sampleBatch(frame, 1);
}

void Audio::reset() {
    _logger = nullptr;
    _coreSampleRate = 0.0;
    _samples.clear();
}
