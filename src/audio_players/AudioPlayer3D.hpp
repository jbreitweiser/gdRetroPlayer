#pragma once

#include <lrcpp/Components.h>

#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>

#include "AudioPlayer.hpp"

class AudioPlayer3D : public AudioPlayer {
public:
    void present(godot::PackedVector2Array* samples);
    void init(godot::AudioStreamPlayer3D* player, double coreSampleRate);

private:
    godot::AudioStreamPlayer3D* _audio_player;
    godot::Ref<godot::AudioStreamGenerator> _audio_generator;
    godot::AudioStreamGeneratorPlayback* _audio_playback;
};
