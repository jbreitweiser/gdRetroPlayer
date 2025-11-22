#include "Video.h"

Video::Video() {
    reset();
}

bool Video::init(Config* config, lrcpp::Logger* logger) {
    reset();

    _logger = logger;

    _logger->info("[Video::init] Video subsystem initialized");

    return true;
}

void Video::destroy() {
    _logger->debug("Video::destroy");
    _frame_buffer.unref();
    reset();
}

double Video::getCoreFps() const {
    _logger->debug("Video::getCoreFps");
    return _coreFps;
}

void Video::clear() {
    _logger->debug("Video::clear");
}

// Copy data from intermediary buffer to Image buffer and update TextureRect
void Video::present() {
    if (_frame_buffer.is_null() || !_frame_buffer.is_valid()) {
        return;
    }

    // Update internal Image with the latest pixel data
    _frame_buffer->set_data(_frame_buffer->get_width(), _frame_buffer->get_height(), false, _frame_buffer->get_format(), _intermediary_buffer);

    // Ensure we have an ImageTexture to update
    if (_image_texture.is_null() || !_image_texture.is_valid()) {
        _image_texture = godot::ImageTexture::create_from_image(_frame_buffer);
        if (_image_texture.is_null() || !_image_texture.is_valid()) {
            _logger->error("Video::present :: Failed to create ImageTexture");
            return;
        }

        // Assign texture to TextureRect if available
        if (_texture_rect != nullptr) {
            _texture_rect->set_texture(_image_texture);
        }
    }
    else {
        // Update existing texture with new image data (more efficient)
        _image_texture->update(_frame_buffer);
    }
}

void Video::set_texture_rect(godot::TextureRect *rect) {
    _texture_rect = rect;
    // If we already have an image texture, assign it immediately
    if (_image_texture.is_valid() && _texture_rect != nullptr) {
        _texture_rect->set_texture(_image_texture);
    }
}

bool Video::setRotation(unsigned rotation) {
    (void)rotation;
    _logger->warn("RETRO_ENVIRONMENT_SET_ROTATION not implemented");
    return false;
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
    _pixelFormat = format;

    switch (_pixelFormat) {
        case RETRO_PIXEL_FORMAT_0RGB1555: 
            _logger->info("[Video::setPixelFormat] Pixel format set to RETRO_PIXEL_FORMAT_0RGB1555"); 
            _textureFormat = godot::Image::Format::FORMAT_RGB565;
            return true;
        case RETRO_PIXEL_FORMAT_XRGB8888: 
            _logger->info("[Video::setPixelFormat] Pixel format set to RETRO_PIXEL_FORMAT_XRGB8888"); 
            _textureFormat = godot::Image::Format::FORMAT_RGBA8;
            return true;
        case RETRO_PIXEL_FORMAT_RGB565: 
            _logger->info("[Video::setPixelFormat] Pixel format set to RETRO_PIXEL_FORMAT_RGB565"); 
            _textureFormat = godot::Image::Format::FORMAT_RGB565;
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
    _aspectRatio = geometry->aspect_ratio;

    if (_aspectRatio <= 0) {
        _aspectRatio = (float)geometry->base_width / (float)geometry->base_height;
    }

    _logger->info("[Video::setGeometry] Core aspect ratio set to %f", _aspectRatio);

    if (_frame_buffer != nullptr) {
        if (geometry->max_width <= _textureWidth && geometry->max_height <= _textureHeight) {
            return true;
        }

        _frame_buffer.unref();
        
        _textureWidth = _textureHeight = 0;
        // Invalidate cached ImageTexture so it will be recreated for the new size
        _image_texture.unref();
    }

    godot::Image::Format format = godot::Image::Format::FORMAT_RGB565;  

    switch (_pixelFormat) {
        case RETRO_PIXEL_FORMAT_0RGB1555: format = godot::Image::Format::FORMAT_RGB565; break;
        case RETRO_PIXEL_FORMAT_XRGB8888: format = godot::Image::Format::FORMAT_RGBA8; break;
        case RETRO_PIXEL_FORMAT_RGB565: format = godot::Image::Format::FORMAT_RGB565; break;

        default:
            _logger->error("[Video::setGeometry] Unknown pixel format, cannot create texture");
            return false;
    }
    
    _frame_buffer = godot::Image::create( geometry->base_width, geometry->base_height, false,
                                               format );
    _intermediary_buffer.resize( geometry->base_width * geometry->base_height *
                            ( format == godot::Image::Format::FORMAT_RGB565 ? 2 : 4 ) );

    if (_frame_buffer == nullptr) {
        _logger->error("[Video::setGeometry] _frame_buffer failed: to create");
        return false;
    }

    _textureFormat  = format;
    _textureWidth = _frame_buffer->get_width();
    _textureHeight = _frame_buffer->get_height();

    _logger->info("[Video::setGeometry] Texture created with %d x %d", _textureWidth, _textureHeight);
    return true;
}

// RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
bool Video::getCurrentSoftwareFramebuffer(retro_framebuffer* framebuffer) {
    if ((framebuffer->access_flags & RETRO_MEMORY_ACCESS_READ) != 0) {
        _logger->debug("[Video::getCurrentSoftwareFramebuffer] Software framebuffer doesn't support reading");
        return false;
    }

    if (framebuffer->width != _textureWidth || framebuffer->height != _textureHeight) {
        // SDL_DestroyTexture(_texture);
        // _texture = nullptr;

        retro_game_geometry geometry;
        geometry.base_width = geometry.max_width = _usedWidth = framebuffer->width;
        geometry.base_height = geometry.max_height = _usedHeight = framebuffer->height;
        geometry.aspect_ratio = _aspectRatio; // maintain the aspect ration

        if (!setGeometry(&geometry)) {
            return false;
        }
    }

    void* texturePixels = nullptr;
    int texturePitch = 0;
    unsigned buffer_size;
    switch ( _frame_buffer->get_format() )
    {
        case godot::Image::FORMAT_RGB565:
            buffer_size =  2;
            break;
        case godot::Image::FORMAT_RGBA8:
            buffer_size = 4;
            break;
        default:
            _logger->error( "[Video::getCurrentSoftwareFramebuffer] Unhandled pixel format: ",
                                               _frame_buffer->get_format() );
            return false;
    }

    buffer_size = framebuffer->width * framebuffer->height * buffer_size;

    _intermediary_buffer.resize( buffer_size );
    texturePixels = (void *)_intermediary_buffer.ptr();
    //memcpy( (void *)_intermediary_buffer.ptr(), data, buffer_size );

    framebuffer->data = texturePixels;
    framebuffer->pitch = texturePitch;
    framebuffer->format = RETRO_PIXEL_FORMAT_RGB565;
    framebuffer->memory_flags = RETRO_MEMORY_ACCESS_WRITE | RETRO_MEMORY_TYPE_CACHED;

    _logger->debug("[Video::getCurrentSoftwareFramebuffer] Returning software framebuffer %p", texturePixels);
    return true;
}

bool Video::getHwRenderInterface(retro_hw_render_interface const** interface) {
    (void)interface;
    _logger->warn("RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE not implemented");
    return false;
}

bool Video::setHwRenderContextNegotiationInterface(retro_hw_render_context_negotiation_interface const* interface) {
    (void)interface;
    _logger->warn("RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE not implemented");
    return false;
}

bool Video::setHwSharedContext() {
    _logger->warn("RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT not implemented");
    return false;
}

bool Video::getTargetRefreshRate(float* rate) {
    // TODO return the real monitor refresh rate?
    *rate = 60.0f;
    return true;
}

bool Video::getPreferredHwRender(unsigned* preferred) {
    *preferred = RETRO_HW_CONTEXT_NONE;
    return true;
}

// retro_video_refresh
void Video::refresh(void const* data, unsigned width, unsigned height, size_t pitch) {
    if ( !data || _frame_buffer.is_null() || !_frame_buffer.is_valid() )
    {
        return;
    }

    if ( (unsigned)_frame_buffer->get_width() != width ||
         (unsigned)_frame_buffer->get_height() != height )
    {
        _logger->info( "Video::refresh :: Resizing frame buffer to %u x %u", width, height );
        auto created_frame_buffer =
            godot::Image::create( width, height, false, _frame_buffer->get_format() );
        if ( created_frame_buffer.is_null() || !created_frame_buffer.is_valid() )
        {
            _logger->error( "Video::refresh :: Failed to recreate frame buffer" );
            return;
        }
        _frame_buffer = created_frame_buffer;
    }

    unsigned buffer_size;
    switch ( _frame_buffer->get_format() )
    {
        case godot::Image::FORMAT_RGB565:
            buffer_size = width * height * 2;
            break;
        
        case godot::Image::FORMAT_RGBA8:
        {
            buffer_size = width * height * 4;

            // Retroarch uses XRGB8888, X is the A, so it looks like ARGB8888, but Godot only
            // supports RGBA8888 We need to swap the first and last bytes, so alpha is the last byte
            // to get accurate color
            uint32_t *data32 = (uint32_t *)data;
            for ( unsigned i = 0; i < width * height; i++ )
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
            _logger->error( "Video::refresh :: Unhandled pixel format: ",
                                               _frame_buffer->get_format() );
            return;
    }
    frame_count++;

    _intermediary_buffer.resize( buffer_size );
    memcpy( (void *)_intermediary_buffer.ptr(), data, buffer_size );
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

    _frame_buffer.unref();
    _image_texture.unref();

    _pixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;
    _coreFps = 0.0;
    _aspectRatio = 0.0f;

    _textureWidth = _textureHeight = 0;
    _textureFormat = godot::Image::Format::FORMAT_RGB565;
    _usedWidth = _usedHeight = 0;

}
