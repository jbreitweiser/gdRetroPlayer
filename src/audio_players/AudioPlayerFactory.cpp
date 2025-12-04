#include "AudioPlayerFactory.hpp"

AudioPlayer* AudioPlayerFactory::createAudioPlayer(Node* node, double coreSampleRate) {

    if (node == nullptr) {
        return nullptr;
    }

    if (AudioStreamPlayer2D* player = Object::cast_to<AudioStreamPlayer2D>(node)) {
        AudioPlayer2D* audio_player = new AudioPlayer2D();
        audio_player->init( player, coreSampleRate );
        return (AudioPlayer*)audio_player;
    } else if (AudioStreamPlayer3D* player = Object::cast_to<AudioStreamPlayer3D>(node)) {
        AudioPlayer3D* audio_player = new AudioPlayer3D();
        audio_player->init( player, coreSampleRate );
        return (AudioPlayer*)audio_player;
    } else {
        // Handle other types or the base Object type
        _logger->error("Audio Player is of an unhandled type.");
        return nullptr;
    }
}

