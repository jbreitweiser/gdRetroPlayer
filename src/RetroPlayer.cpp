#include "RetroPlayer.hpp"

#include <errno.h>
#include <sys/stat.h>


bool RetroPlayer::loadCore(char const *path)
{
    if (!_dynlib.load(path)) {
        _logger.error("RetroPlayer::loadCore :: Failed to load library: ");
        return false;
    }

#define LOAD_CORE_FUNC(member_name, name) \
    _core.member_name = reinterpret_cast<decltype(_core.member_name)>(_dynlib.getSymbol("retro_" #name)); \
    if (!_core.member_name) { \
        _logger.error("RetroPlayer::loadCore :: Missing symbol: retro_" #name); \
        return false; \
    }

    LOAD_CORE_FUNC(init, init)
    LOAD_CORE_FUNC(deinit, deinit)
    LOAD_CORE_FUNC(apiVersion, api_version)
    LOAD_CORE_FUNC(getSystemInfo, get_system_info)
    LOAD_CORE_FUNC(getSystemAvInfo, get_system_av_info)
    LOAD_CORE_FUNC(setEnvironment, set_environment)
    LOAD_CORE_FUNC(setVideoRefresh, set_video_refresh)
    LOAD_CORE_FUNC(setAudioSample, set_audio_sample)
    LOAD_CORE_FUNC(setAudioSampleBatch, set_audio_sample_batch)
    LOAD_CORE_FUNC(setInputPoll, set_input_poll)
    LOAD_CORE_FUNC(setInputState, set_input_state)
    LOAD_CORE_FUNC(setControllerPortDevice, set_controller_port_device)
    LOAD_CORE_FUNC(reset, reset)
    LOAD_CORE_FUNC(run, run)
    LOAD_CORE_FUNC(serializeSize, serialize_size)
    LOAD_CORE_FUNC(serialize, serialize)
    LOAD_CORE_FUNC(unserialize, unserialize)
    LOAD_CORE_FUNC(cheatReset, cheat_reset)
    LOAD_CORE_FUNC(cheatSet, cheat_set)
    LOAD_CORE_FUNC(loadGame, load_game)
    LOAD_CORE_FUNC(loadGameSpecial, load_game_special)
    LOAD_CORE_FUNC(unloadGame, unload_game)
    LOAD_CORE_FUNC(getRegion, get_region)
    LOAD_CORE_FUNC(getMemoryData, get_memory_data)
    LOAD_CORE_FUNC(getMemorySize, get_memory_size)

#undef LOAD_CORE_FUNC

    // If needed: keep lib alive by returning it or capturing in a smart pointer
    return true;
}

RetroPlayer::RetroPlayer()
{
    _frontend.setLogger(&_logger);
    _logger.info( "[RetroPlayer] Constructor" );
}

RetroPlayer::~RetroPlayer()
{
    destroy();
    _logger.info( "[RetroPlayer] Destructor" );
}

bool RetroPlayer::init(std::vector<std::string> const& configPaths, char const* corePath, char const* contentPath, int verboseness) {

    _logger.setLevel(RETRO_LOG_WARN);
    
    if (!_config.init(configPaths, contentPath, corePath, &_logger)) {
        _logger.error("RetroPlayer::init :: Could not initialize the configuration component");
        return false;
    }

    {
        unsigned long level = 3;
        _config.getOption("libretro_log_level", &level);
        long realLevel = (long)level - verboseness;

        if (realLevel <= 0) {
            _logger.setLevel(RETRO_LOG_DEBUG);
        }
        else if (realLevel == 1) {
            _logger.setLevel(RETRO_LOG_INFO);
        }
        else if (realLevel == 2) {
            _logger.setLevel(RETRO_LOG_WARN);
        }
        else if (realLevel >= 3) {
            _logger.setLevel(RETRO_LOG_ERROR);
        }
    }

    if (!_perf.init(&_logger)) {
        _logger.error("RetroPlayer::init :: Could not initialize the perf component");
        _config.destroy();
        return false;
    }

    if (!_audio.init(&_config, &_logger)) {
        _logger.error("RetroPlayer::init :: Could not initialize the audio component");
        _perf.destroy();
        _config.destroy();
        return false;
    }

    if (!_video.init(&_config, &_logger)) {
        _logger.error("RetroPlayer::init :: Could not initialize the video component");
        _audio.destroy();
        _perf.destroy();
        _config.destroy();
        return false;
    }

    if (!_input.init(&_logger)) {
        _logger.error("RetroPlayer::init :: Could not initialize the input component");
        _video.destroy();
        _audio.destroy();
        _perf.destroy();
        _config.destroy();
        return false;
    }

    if (!_frontend.setLogger(&_logger) || !_frontend.setConfig(&_config) || !_frontend.setVideo(&_video)) {
        _logger.error("RetroPlayer::init :: Could not set components in the frontend");

error:
        _input.destroy();
        _video.destroy();
        _audio.destroy();
        _perf.destroy();
        _config.destroy();
        return false;
    }

    if (!_frontend.setPerf(&_perf) || !_frontend.setAudio(&_audio) || !_frontend.setInput(&_input)) {
        _logger.error("RetroPlayer::init :: Could not set components in the frontend");
        goto error;
    }

    _logger.info("RetroPlayer::init :: Loading core from \"%s\"", corePath);

    if (!loadCore(corePath)) {
        _logger.error("RetroPlayer::init :: Failed to initialize core.");
        goto error;
    }

    if (!_frontend.setCore(&_core)) {
        _logger.error("RetroPlayer::init :: Could not load the core from \"%s\"", corePath);
        _dynlib.unload();
        goto error;
    }

    retro_system_info sysinfo;

    if (!_frontend.getSystemInfo(&sysinfo)) {
        _logger.error("RetroPlayer::init :: Could not get the system info from the core");
        _frontend.unset();
        _dynlib.unload();
        goto error;
    }

    _logger.info("RetroPlayer::init :: System Info");
    _logger.info("    library_name     = %s", sysinfo.library_name);
    _logger.info("    library_version  = %s", sysinfo.library_version);
    _logger.info("    valid_extensions = %s", sysinfo.valid_extensions);
    _logger.info("    need_fullpath    = %s", sysinfo.need_fullpath ? "true" : "false");
    _logger.info("    block_extract    = %s", sysinfo.block_extract ? "true" : "false");

    _logger.info("Loading content from \"%s\"", contentPath);

    bool ok = false;

    if (sysinfo.need_fullpath) {
        ok = _frontend.loadGame(contentPath);
    }
    else {
        size_t size = 0;
        void const* data = readAll(contentPath, &size);

        if (data != nullptr) {
            ok = _frontend.loadGame(contentPath, data, size);
            std::free(const_cast<void*>(data));
        }
    }

    if (!ok) {
        _logger.error("RetroPlayer::init :: Could not load content from \"%s\"", contentPath);
        _frontend.unset();
        _dynlib.unload();
        goto error;
    }

    _initialized = true;
    return true;
}

bool RetroPlayer::player_init(const godot::PackedStringArray &configPaths, const godot::String &corePath, const godot::String &contentPath, int verboseness) {
    std::vector<std::string> paths;
    paths.reserve(configPaths.size());
    for (int i = 0; i < (int)configPaths.size(); ++i) {
        godot::String s = configPaths[i];
        std::string ss = s.utf8().get_data();
        paths.push_back(ss);
    }

    std::string core = corePath.utf8().get_data();
    std::string content = contentPath.utf8().get_data(); 

    return init(paths, core.c_str(), content.c_str(), verboseness);
}

void RetroPlayer::destroy() {
    _frontend.unloadGame();
    _frontend.unset();

    _input.destroy();
    _video.destroy();
    _audio.destroy();
    _config.destroy();
    _perf.destroy();

    _dynlib.unload();
}

void RetroPlayer::run() {
    if( _initialized) {
        _audio.clear();
        _video.clear();
        _frontend.run();
        _audio.present();
        _video.present();  // Copy framebuffer data to the texture and render it
    } else {
        _logger.error("RetroPlayer::run :: Player not initialized correctly.");
    }
}

void RetroPlayer::forwarded_input( const godot::Ref<godot::InputEvent> &event )
{
    if(_initialized)
        _input.process( event.ptr() );
}

void RetroPlayer::input( const Dictionary &event )
{
    if(_initialized)
        _input.process( event );
}

void const* RetroPlayer::readAll(char const* path, size_t* size) {
    struct stat statbuf;

    if (stat(path, &statbuf) != 0) {
        _logger.error("RetroPlayer::readAll :: Error getting content info: %s", strerror(errno));
        return nullptr;
    }

    void* data = malloc(statbuf.st_size);

    if (data == nullptr) {
        _logger.error("RetroPlayer::readAll :: Out of memory allocating %zu bytes", statbuf.st_size);
        return nullptr;
    }

    FILE* file = fopen(path, "rb");

    if (file == nullptr) {
        _logger.error("RetroPlayer::readAll :: Error opening content: %s", strerror(errno));
        std::free(data);
        return nullptr;
    }

    size_t numread = fread(data, 1, statbuf.st_size, file);

    if (numread != (size_t)statbuf.st_size) {
        _logger.error("RetroPlayer::readAll :: Error reading content: %s", strerror(errno));
        fclose(file);
        std::free(data);
        return nullptr;
    }

    fclose(file);

    _logger.debug("RetroPlayer::readAll :: Loaded data from \"%s\", %zu bytes", path, numread);
    *size = numread;
    return data;
}

void RetroPlayer::set_texture_rect(godot::TextureRect *rect) {
    _video.set_texture_rect(rect);
}

void RetroPlayer::set_render_surface(godot::Node *node) {
    _video.set_render_surface(node);
}

void RetroPlayer::set_audio_player(godot::Node *player) {
    _audio.set_audio_player(player);
}

double RetroPlayer::getCoreSampleRate() {
    return _audio.getCoreSampleRate();
}

void RetroPlayer::_bind_methods()
{   
    godot::ClassDB::bind_method( godot::D_METHOD( "player_init", "configPaths", "corePath", "contentPath", "verboseness" ), &RetroPlayer::player_init );
    godot::ClassDB::bind_method( godot::D_METHOD( "run" ), &RetroPlayer::run );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_render_surface", "node" ), &RetroPlayer::set_render_surface );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_texture_rect", "texture_rect" ), &RetroPlayer::set_texture_rect );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_audio_player", "player" ), &RetroPlayer::set_audio_player );
    godot::ClassDB::bind_method( godot::D_METHOD( "forward_input", "event" ), &RetroPlayer::forwarded_input );
    godot::ClassDB::bind_method( godot::D_METHOD( "input", "event" ), &RetroPlayer::input );
    godot::ClassDB::bind_method( godot::D_METHOD( "quit" ), &RetroPlayer::destroy );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_sample_rate" ), &RetroPlayer::getCoreSampleRate );
    
}
