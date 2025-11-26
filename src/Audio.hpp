#pragma once

#include "Config.hpp"

#include <lrcpp/Components.h>

#include <string>
#include <vector>
#include <stdint.h>
#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

class Audio : public lrcpp::Audio {
public:
    Audio();

    bool init(Config* config, lrcpp::Logger* logger);
    void destroy();

    double getCoreSampleRate() const;

    void clear();
    void present();
    void set_audio_player(godot::AudioStreamPlayer2D *player);

    // lrcpp::Audio
    virtual bool setSystemAvInfo(retro_system_av_info const* info) override;
    virtual bool setAudioCallback(retro_audio_callback const* callback) override;
    virtual size_t sampleBatch(int16_t const* data, size_t frames) override;
    virtual void sample(int16_t left, int16_t right) override;

protected:
    void reset();

    lrcpp::Logger* _logger;
    double _coreSampleRate;
    godot::PackedVector2Array _samples;

    // Godot audio output
    godot::AudioStreamPlayer2D* _audio_player;
    godot::Ref<godot::AudioStreamGenerator> _audio_generator;
    godot::AudioStreamGeneratorPlayback* _audio_playback;
};
