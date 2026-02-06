#pragma once

#include "Logger.hpp"
#include "Perf.hpp"
#include "Config.hpp"
#include "Audio.hpp"
#include "Video.hpp"
#include "Input.hpp"
#include "DynLib.hpp"

#include "lrcpp/Frontend.h"

#include <string>
#include <vector>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>

using namespace godot;

class RetroPlayer : public RefCounted {
    GDCLASS(RetroPlayer, RefCounted)

public:
    RetroPlayer();
	~RetroPlayer() override;

    bool init(std::vector<std::string> const& configPaths, char const* corePath, char const* contentPath, int verboseness);
    // Godot-facing wrapper that accepts Godot types and converts them to STL types
    bool player_init(const PackedStringArray &configPaths, const godot::String &corePath, const godot::String &contentPath, int verboseness);
    bool set_config(const godot::PackedStringArray &configPaths, const godot::String &corePath, const godot::String &contentPath, int verboseness);
    bool set_option(godot::String const& key, godot::String const& value);
    godot::Array get_core_options();
    bool retro_init();
    void set_log_level(int verboseness);
    Dictionary load_core();
    bool load_content();
    void destroy();
    void run();
    void forwarded_input( const Ref<InputEvent> &event );
    void input( const Dictionary &event );
    double getCoreSampleRate();
    void set_texture_rect(TextureRect *rect);
    void set_render_surface(godot::Node *node);
    void set_audio_player(Node *player);

protected:
    bool _initialized = false;
    bool _core_loaded = false;
    bool _content_loaded = false;
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

    Dictionary _system_info = Dictionary();

    static void _bind_methods();
    
};
