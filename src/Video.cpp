#include "Video.hpp"
#include "render_surfaces/RenderSurfaceFactory.hpp"

Video::Video() {
    reset();
}

Video::~Video() {
    destroy();
    if(_renderer != nullptr) {
        delete _renderer;
        _renderer = nullptr;
    }
}

bool Video::init(Config* config, lrcpp::Logger* logger) {
    reset();

    _logger = logger;

    return true;
}

void Video::destroy() {
    reset();
}

double Video::getCoreFps() const {
    _logger->debug("Video::getCoreFps");
    return _coreFps;
}

void Video::clear() {
   // _logger->debug("Video::clear");
}

// Copy data from intermediary buffer to Image buffer and update TextureRect
void Video::present() {
    _renderer->present(&_intermediary_buffer, _frameWidth, _frameHeight, _rotation);
}


void Video::set_render_surface(godot::Node *node) {
    RenderSurfaceFactory* factory = new RenderSurfaceFactory(_logger);
    _renderer = factory->createRenderer(node);
}


//  Rename to set render surface and use class name to determine which render class to apply
void Video::set_texture_rect(godot::TextureRect *rect) {
    set_render_surface((godot::Node*) rect); 

}

// RETRO_ENVIRONMENT_SET_ROTATION
bool Video::setRotation(unsigned rotation) {
    _logger->warn("[Video::setRotation] Set Rotation Called");
    _rotation = rotation;
    return true;
}

bool Video::getOverscan(bool* overscan) {
    (void)overscan;
    _logger->warn("RETRO_ENVIRONMENT_GET_OVERSCAN not implemented");
    return false;
}

bool Video::getCanDupe(bool* canDupe) {
    _logger->debug("Video::getCanDupe");
    *canDupe = true;
    return true;
}

bool Video::showMessage(retro_message const* message) {
    _logger->warn("RETRO_ENVIRONMENT_SET_MESSAGE not implemented (%u, \"%s\")", message->frames, message->msg);
    return true;
}

bool Video::setPixelFormat(retro_pixel_format format) {
    _logger->warn("[Video::setGeometry] Set Pixel Format Called");
    _pixelFormat = format;

    switch (_pixelFormat) {
        case RETRO_PIXEL_FORMAT_0RGB1555: 
            _renderer->setPixelFormat(godot::Image::Format::FORMAT_RGB565);
            _logger->info("[Video::setPixelFormat] Pixel format set to RETRO_PIXEL_FORMAT_0RGB1555"); 
            return true;
        case RETRO_PIXEL_FORMAT_XRGB8888: 
            _renderer->setPixelFormat(godot::Image::Format::FORMAT_RGBA8);
            _logger->info("[Video::setPixelFormat] Pixel format set to RETRO_PIXEL_FORMAT_XRGB8888"); 
            return true;
        case RETRO_PIXEL_FORMAT_RGB565: 
            _renderer->setPixelFormat(godot::Image::Format::FORMAT_RGB565);
            _logger->info("[Video::setPixelFormat] Pixel format set to RETRO_PIXEL_FORMAT_RGB565"); 
            return true;
        default: 
            return false;
    }
}


bool Video::setHwRender(retro_hw_render_callback* callback) {
    (void)callback;
    _logger->warn("RETRO_ENVIRONMENT_SET_HW_RENDER not implemented");
    return false;
}

bool Video::setFrameTimeCallback(retro_frame_time_callback const* callback) {
    (void)callback;
    _logger->warn("RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK not implemented");
    return false;
}

// RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO
//  Called to set the godot video environment
bool Video::setSystemAvInfo(retro_system_av_info const* info) {
    _coreFps = info->timing.fps;
    _logger->info("[Video::setSystemAvInfo] Core FPS set to %f", _coreFps);
    return setGeometry(&info->geometry);
}

// RETRO_ENVIRONMENT_SET_GEOMETRY
bool Video::setGeometry(retro_game_geometry const* geometry) {
    _logger->warn("[Video::setGeometry] Set Geometry Called");
    unsigned int channels;
    godot::Image::Format format = godot::Image::Format::FORMAT_RGB565;

    _aspectRatio = geometry->aspect_ratio;
    _frameWidth = geometry->base_width;
    _frameHeight = geometry->base_height;
    _textureWidth = geometry->max_width;
    _textureHeight = geometry->max_height;

    // If aspect ratio is not set, calculate it
    if (_aspectRatio <= 0) {
        _aspectRatio = (float)geometry->base_width / (float)geometry->base_height;
    }

    return _renderer->setFrameBuffer(_frameWidth, _frameHeight);
    return false;
}

// RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
bool Video::getCurrentSoftwareFramebuffer(retro_framebuffer* framebuffer) {
    _logger->warn("RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER not implemented");
    return false;
}

// RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE
bool Video::getHwRenderInterface(retro_hw_render_interface const** interface) {
    (void)interface;
    _logger->warn("RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE not implemented");
    return false;
}

//  RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE
bool Video::setHwRenderContextNegotiationInterface(retro_hw_render_context_negotiation_interface const* interface) {
    (void)interface;
    _logger->warn("RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE not implemented");
    return false;
}

// RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT
bool Video::setHwSharedContext() {
    _logger->warn("RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT not implemented");
    return false;
}

bool Video::getTargetRefreshRate(float* rate) {
    godot::DisplayServer* server = godot::DisplayServer::get_singleton();
    int screen_id = server->window_get_current_screen();
    *rate  = server->screen_get_refresh_rate(screen_id);
    if (*rate  < 0) {
        *rate  =  60.0f; // Default to 60 Hz
    }

    _logger->warn("Video::getTargetRefreshRate :: Monitor refresh rate is %f Hz", *rate);
    return true;
}

bool Video::getPreferredHwRender(unsigned* preferred) {
    *preferred = RETRO_HW_CONTEXT_NONE;
    return true;
}

// retro_video_refresh
void Video::refresh(void const* data, unsigned width, unsigned height, size_t pitch) {
    unsigned int channels;
    unsigned buffer_size;

    if ( !data )
    {
        return;
    }
    _pitch = pitch;
    _frameWidth = width;
    _frameHeight = height;

    if(!_renderer->setFrameBuffer(_frameWidth, _frameHeight)) {
        return;
    }

    godot::Image::Format format = _renderer->get_format();
    switch ( format )
    {
        case godot::Image::FORMAT_RGB565:
            channels = 2;
            break;
        case godot::Image::FORMAT_RGBA8:
        {
            channels = 4;
            
            // Retroarch uses XRGB8888, X is the A, so it looks like ARGB8888, but Godot only
            // supports RGBA8888 We need to swap the first and last bytes, so alpha is the last byte
            // to get accurate color
            uint32_t *data32 = (uint32_t *)data;
            for ( unsigned i = 0; i < (_pitch/channels) * _frameHeight; i++ )
            {
                uint32_t pixel = data32[i];
                // Force alpha to fully opaque (1.0 -> 255) so Godot receives an opaque image
                uint8_t alpha = 0xFF;
                uint8_t red = ( pixel & 0x00FF0000 ) >> 16;
                uint8_t green = ( pixel & 0x0000FF00 ) >> 8;
                uint8_t blue = ( pixel & 0x000000FF );
                data32[i] = ( alpha << 24 ) | ( blue << 16 ) | ( green << 8 ) | red;
            }
        }
        break;
        default:
            _logger->error( "Video::refresh :: Unhandled pixel format: ", format );
            return;
    }

    frame_count++;
    // depending on rotation the pitch needs to change.  
    // The passed pitch will change if the width and height change.
    int dest_pitch = width * channels;
    buffer_size = dest_pitch * height;

    if (buffer_size > _intermediary_buffer.size()) {
        _intermediary_buffer.resize( buffer_size );
    }

    copy_frame_to_buffer(data, pitch, _intermediary_buffer, 
                        dest_pitch, width, height);
    
}

void Video::copy_frame_to_buffer(void const* src_data,
                                int src_pitch,
                                godot::PackedByteArray &dest_buffer,
                                int dst_pitch,
                                int frame_width,
                                int frame_height)
{
    // Get destination buffer
    uint8_t *dst_ptr = dest_buffer.ptrw();

    const uint8_t *src_buffer = (const uint8_t *)src_data;

    for (int y = 0; y < frame_height; y++) {
        const uint8_t *src_row = src_buffer + y * src_pitch;
        uint8_t *dst_row = dst_ptr + y * dst_pitch;

        // Copy only the visible width, not the padded pitch
        memcpy(dst_row, src_row, dst_pitch);
    }
}

uintptr_t Video::getCurrentFramebuffer() {
     _logger->info("Video::getCurrentFramebuffer");
    return 0;
}

retro_proc_address_t Video::getProcAddress(char const* symbol) {
    (void)symbol;
    _logger->warn("RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT not implemented");
    return nullptr;
}

void Video::reset() {
    _logger = nullptr;

    _pixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;
    _coreFps = 0.0;
    _aspectRatio = 0.0f;

    _textureWidth = _textureHeight = 0;
}
