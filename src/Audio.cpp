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
    if (_audio_playback == nullptr) {
        return;
    }

    size_t frames =  _samples.size();

    if (frames == 0) {
        return;
    }

    _audio_playback->push_buffer(_samples);

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

void Audio::set_audio_player(godot::AudioStreamPlayer2D *player) {
    _audio_player = player;

    if (_audio_player == nullptr) {
        _audio_generator.unref();
        _audio_playback = nullptr;
        return;
    }

    // instantiate an AudioStreamGenerator and set it as the player's stream
    godot::Ref<godot::AudioStreamGenerator> gen;
    gen.instantiate();
    if (_coreSampleRate > 0.0) {
        gen->set_mix_rate((float)_coreSampleRate);
    }
    // default buffer length
    gen->set_buffer_length(0.5f);

    _audio_generator = gen;
    _audio_player->set_stream(_audio_generator);

    _audio_player->play();
    // obtain playback object (AudioStreamGeneratorPlayback)
    godot::Ref<godot::AudioStreamPlayback> pb = _audio_player->get_stream_playback();
    if (!pb.is_null()) {
        _audio_playback = godot::Object::cast_to<godot::AudioStreamGeneratorPlayback>(pb.ptr());
    } else {
        _audio_playback = nullptr;
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
