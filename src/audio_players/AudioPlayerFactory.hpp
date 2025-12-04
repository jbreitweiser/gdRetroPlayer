#pragma once

#include "AudioPlayer.hpp"

#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>

#include "AudioPlayer2D.hpp"
#include "AudioPlayer3D.hpp"

using namespace godot;

class AudioPlayerFactory {
public:
    AudioPlayerFactory(lrcpp::Logger* logger) {
        _logger = logger;
    }
    ~AudioPlayerFactory() {
        _logger = nullptr;
    }
    AudioPlayer* createAudioPlayer(Node* node, double coreSampleRate);
private:
    lrcpp::Logger* _logger = nullptr;
};
