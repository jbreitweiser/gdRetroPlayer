#include "AudioPlayer3D.hpp"

void AudioPlayer3D::present(godot::PackedVector2Array* samples) {
    // Push queued samples to audio playback if available
    if (_audio_playback == nullptr) {
        return;
    }

    size_t frames =  samples->size();

    if (frames == 0) {
        return;
    }

    _audio_playback->push_buffer(*samples);

    samples->clear();

}

void AudioPlayer3D::init(godot::AudioStreamPlayer3D* player, double coreSampleRate) {
    _audio_player = player;

    if (_audio_player == nullptr) {
        _audio_generator.unref();
        _audio_playback = nullptr;
        return;
    }

    // instantiate an AudioStreamGenerator and set it as the player's stream
    godot::Ref<godot::AudioStreamGenerator> gen;
    gen.instantiate();
    if (coreSampleRate > 0.0) {
        gen->set_mix_rate((float) coreSampleRate);
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
