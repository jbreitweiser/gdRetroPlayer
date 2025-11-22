#pragma once

#include "Logger.h"
#include "Perf.h"
#include "Config.h"
#include "Audio.h"
#include "Video.h"
#include "Input.h"
#include "DynLib.h"

#include "lrcpp/Frontend.h"

#include <string>
#include <vector>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

using namespace godot;

class RetroPlayer : public RefCounted {
    GDCLASS(RetroPlayer, RefCounted)

public:
    RetroPlayer();
	~RetroPlayer() override = default;

    bool init(std::vector<std::string> const& configPaths, char const* corePath, char const* contentPath, int verboseness);
    // Godot-facing wrapper that accepts Godot types and converts them to STL types
    bool player_init(const PackedStringArray &configPaths, const godot::String &corePath, const godot::String &contentPath, int verboseness);
    void destroy();
    void run();
    void forwarded_input( const Ref<InputEvent> &event );
    void input( const Dictionary &event );
    Ref<Image> get_frame_buffer();
    PackedFloat32Array get_audio_samples();
    void clear_audio_buffer();
    double getCoreSampleRate();
    void set_texture_rect(TextureRect *rect);



protected:
    bool done = false;
    void const* readAll(char const* path, size_t* size);
    bool loadCore(char const *path);

    lrcpp::Frontend _frontend;
    Logger _logger;
    Perf _perf;
    Config _config;
    Audio _audio;
    Video _video;
    RetroInput _input;

    lrcpp::Core _core;
    DynLib _dynlib;

    static void _bind_methods();
    
};
