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
    _logger.setLevel(RETRO_LOG_DEBUG);
    retro_init();
}

RetroPlayer::~RetroPlayer()
{
    destroy();
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

    _initialized = true;

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

    _core_loaded = true;

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

bool RetroPlayer::set_config(const godot::PackedStringArray &configPaths, const godot::String &corePath, const godot::String &contentPath, int verboseness){
    std::vector<std::string> paths;
    paths.reserve(configPaths.size());
    for (int i = 0; i < (int)configPaths.size(); ++i) {
        godot::String s = configPaths[i];
        std::string ss = s.utf8().get_data();
        paths.push_back(ss);
    }

    std::string core = corePath.utf8().get_data();
    std::string content = contentPath.utf8().get_data(); 

    if (!_config.init(paths, content.c_str(), core.c_str(), &_logger)) {
        _logger.error("RetroPlayer::init :: Could not initialize the configuration component");
        return false;
    }

    set_log_level(verboseness);

    return true;
}

bool RetroPlayer::set_option(godot::String const& key, godot::String const& value){
    return _config.setOption(key.utf8().get_data(), value.utf8().get_data());
}

godot::Array RetroPlayer::get_core_options(){
    return _config.getCoreOptions();
}

bool RetroPlayer::retro_init(){
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

    if (!_frontend.setLogger(&_logger) || !_frontend.setConfig(&_config) || !_frontend.setVideo(&_video) ||
        !_frontend.setPerf(&_perf) || !_frontend.setAudio(&_audio) || !_frontend.setInput(&_input)) {
        _logger.error("RetroPlayer::init :: Could not set components in the frontend");

        _input.destroy();
        _video.destroy();
        _audio.destroy();
        _perf.destroy();
        _config.destroy();
        return false;
    }

    _initialized = true;

    return true;
}

void RetroPlayer::set_log_level(int verboseness) {
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

Dictionary RetroPlayer::load_core() {
    std::string corePath;
    _system_info.clear();

    if(!_initialized) {
        _logger.error("RetroPlayer::init :: Player not initialized, cannot load core.");
        return _system_info;
    }

    corePath = _config.getCorePath();

    if (corePath.empty()) {
        _logger.error("RetroPlayer::init :: Could not get system directory from config");
        return _system_info;
    }

    _logger.info("RetroPlayer::init :: Loading core from \"%s\"", corePath.c_str());

    if (!loadCore(corePath.c_str())) {
        _logger.error("RetroPlayer::init :: Failed to initialize core.");
        return _system_info;
    }

    if (!_frontend.setCore(&_core)) {
        _logger.error("RetroPlayer::init :: Could not load the core from \"%s\"", corePath.c_str());
        _dynlib.unload();
        return _system_info;
    }

    retro_system_info sysinfo;

    if (!_frontend.getSystemInfo(&sysinfo)) {
        _logger.error("RetroPlayer::init :: Could not get the system info from the core");
        _frontend.unset();
        _dynlib.unload();
        return _system_info;
    }

    _system_info.set("library_name", sysinfo.library_name);
    _system_info.set("library_version", sysinfo.library_version);
    _system_info.set("valid_extensions", sysinfo.valid_extensions);
    _system_info.set("need_fullpath", sysinfo.need_fullpath ? true : false);
    _system_info.set("block_extract", sysinfo.block_extract ? true : false);

    _logger.info("RetroPlayer::init :: System Info");
    _logger.info("    library_name     = %s", sysinfo.library_name);
    _logger.info("    library_version  = %s", sysinfo.library_version);
    _logger.info("    valid_extensions = %s", sysinfo.valid_extensions);
    _logger.info("    need_fullpath    = %s", sysinfo.need_fullpath ? "true" : "false");
    _logger.info("    block_extract    = %s", sysinfo.block_extract ? "true" : "false");

    _core_loaded = true;
    
    return _system_info;
}

bool RetroPlayer::load_content() {
    std::string contentPath = _config.getContentPath();

    if( !_core_loaded) {
        _logger.error("RetroPlayer::load_content :: Core not loaded, cannot load content.");
        return false;
    }

    _logger.info("Loading content from \"%s\"", contentPath.c_str());

    bool ok = false;

    if (_system_info.get("need_fullpath", false)) {
        ok = _frontend.loadGame(contentPath.c_str());
    }
    else {
        size_t size = 0;
        void const* data = readAll(contentPath.c_str(), &size);

        if (data != nullptr) {
            ok = _frontend.loadGame(contentPath.c_str(), data, size);
            std::free(const_cast<void*>(data));
        }
    }

    if (!ok) {
        _logger.error("RetroPlayer::init :: Could not load content from \"%s\"", contentPath.c_str());
        return false;
    }

    _content_loaded = true;

    return true;
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
    _system_info.clear();
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
    godot::ClassDB::bind_method( godot::D_METHOD( "set_log_level", "verboseness" ), &RetroPlayer::set_log_level );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_config", "configPaths", "corePath", "contentPath", "verboseness" ), &RetroPlayer::set_config );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_option", "key", "value" ), &RetroPlayer::set_option );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_options" ), &RetroPlayer::get_core_options );
    godot::ClassDB::bind_method( godot::D_METHOD( "load_core" ), &RetroPlayer::load_core );
    godot::ClassDB::bind_method( godot::D_METHOD( "load_content" ), &RetroPlayer::load_content );
    godot::ClassDB::bind_method( godot::D_METHOD( "run" ), &RetroPlayer::run );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_render_surface", "node" ), &RetroPlayer::set_render_surface );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_texture_rect", "texture_rect" ), &RetroPlayer::set_texture_rect );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_audio_player", "player" ), &RetroPlayer::set_audio_player );
    godot::ClassDB::bind_method( godot::D_METHOD( "forward_input", "event" ), &RetroPlayer::forwarded_input );
    godot::ClassDB::bind_method( godot::D_METHOD( "input", "event" ), &RetroPlayer::input );
    godot::ClassDB::bind_method( godot::D_METHOD( "quit" ), &RetroPlayer::destroy );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_sample_rate" ), &RetroPlayer::getCoreSampleRate );
    
}
