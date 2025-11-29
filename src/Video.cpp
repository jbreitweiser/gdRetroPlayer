#include "Video.hpp"

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
    _logger->warn("[Video::setGeometry] Set Geometry Called");
    unsigned int channels;
    godot::Image::Format format = godot::Image::Format::FORMAT_RGB565;

    _aspectRatio = geometry->aspect_ratio;
    _usedWidth = _textureWidth = geometry->base_width;
    _usedHeight = _textureHeight = geometry->base_height;

    // If aspect ratio is not set, calculate it
    if (_aspectRatio <= 0) {
        _aspectRatio = (float)geometry->base_width / (float)geometry->base_height;
    }

    // Adjust width and height for rotation
    if(_rotation == 1 || _rotation == 3) {
        std::swap(_textureWidth, _textureHeight);
    }

    // // If we already have a frame buffer, check if size matches
    // if (_frame_buffer != nullptr) {
    //     if (_textureWidth == _lastTextureWidth && _textureHeight == _lastTextureHeight) {
    //         return true;
    //     }

    //     _frame_buffer.unref();
    //     // Invalidate cached ImageTexture so it will be recreated for the new size
    //     _image_texture.unref();
    // }

    // // Determine Godot pixel format based on retro pixel format 
    // switch (_pixelFormat) {
    //     case RETRO_PIXEL_FORMAT_0RGB1555: 
    //         channels = 2;
    //         format = godot::Image::Format::FORMAT_RGB565; 
    //         break;
    //     case RETRO_PIXEL_FORMAT_XRGB8888: 
    //         channels = 4;
    //         format = godot::Image::Format::FORMAT_RGBA8; 
    //         break;
    //     case RETRO_PIXEL_FORMAT_RGB565: 
    //         channels = 2;
    //         format = godot::Image::Format::FORMAT_RGB565;
    //         break;
    //     default:
    //         _logger->error("[Video::setGeometry] Unknown pixel format, cannot create texture");
    //         return false;
    // }
    
    // _frame_buffer = godot::Image::create( _textureWidth, _textureHeight, false, format );
    // _intermediary_buffer.resize( _textureWidth * _textureHeight * channels );

    // if (_frame_buffer == nullptr) {
    //     _logger->error("[Video::setGeometry] _frame_buffer failed: to create");
    //     return false;
    // }

    // _textureFormat  = format;
    // _lastTextureWidth = _frame_buffer->get_width();
    // _lastTextureHeight = _frame_buffer->get_height();

    // _logger->info("[Video::setGeometry] Texture created with %d x %d", _textureWidth, _textureHeight);
    return setFrameBuffer();
}

// if there are changes in rotation, we may need to adjust the geometry
bool Video::setFrameBuffer() {
    unsigned int channels;
    godot::Image::Format format = godot::Image::Format::FORMAT_RGB565;

    // Adjust width and height for rotation
    if(_rotation == 1 || _rotation == 3) {
        std::swap(_textureWidth, _textureHeight);
    }

    // If we already have a frame buffer, check if size matches
    if (_frame_buffer != nullptr) {
        if (_textureWidth == _lastTextureWidth && _textureHeight == _lastTextureHeight) {
            return true;
        }

        // Invalidate framwbuffer and ImageTexture so it will be recreated for the new size
        _frame_buffer.unref();
        _image_texture.unref();
    }
    
     // Determine Godot pixel format based on retro pixel format 
    switch (_pixelFormat) {
        case RETRO_PIXEL_FORMAT_0RGB1555: 
            channels = 2;
            format = godot::Image::Format::FORMAT_RGB565; 
            break;
        case RETRO_PIXEL_FORMAT_XRGB8888: 
            channels = 4;
            format = godot::Image::Format::FORMAT_RGBA8; 
            break;
        case RETRO_PIXEL_FORMAT_RGB565: 
            channels = 2;
            format = godot::Image::Format::FORMAT_RGB565;
            break;
        default:
            _logger->error("[Video::setGeometry] Unknown pixel format, cannot create texture");
            return false;
    }
    
    _frame_buffer = godot::Image::create( _textureWidth, _textureHeight, false, format );
    _intermediary_buffer.resize( _textureWidth * _textureHeight * channels );

    if (_frame_buffer == nullptr) {
        _logger->error("[Video::setGeometry] _frame_buffer failed: to create");
        return false;
    }

    _textureFormat  = format;
    _lastTextureWidth = _frame_buffer->get_width();
    _lastTextureHeight = _frame_buffer->get_height();

    return true;
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
    // TODO return the real monitor refresh rate?
    *rate = get_current_monitor_refresh_rate();
    _logger->warn("Video::getTargetRefreshRate :: Monitor refresh rate is %f Hz", *rate);
    return true;
}

float Video::get_current_monitor_refresh_rate() {
    godot::DisplayServer* server = godot::DisplayServer::get_singleton();
    // Get the ID of the screen where the main window is currently located
    int screen_id = server->window_get_current_screen();
    
    // Get the refresh rate for that specific screen
    float refresh_rate = server->screen_get_refresh_rate(screen_id);

    // If the refresh rate cannot be found (e.g., on Web platforms), it returns -1.0
    if (refresh_rate < 0) {
        // Handle error or use a default value
        return 60.0f; // Default to 60 Hz
    }
    
    return refresh_rate;
}

bool Video::getPreferredHwRender(unsigned* preferred) {
    *preferred = RETRO_HW_CONTEXT_NONE;
    return true;
}

// retro_video_refresh
void Video::refresh(void const* data, unsigned width, unsigned height, size_t pitch) {
    unsigned int channels;
    unsigned buffer_size;

    if ( !data || _frame_buffer.is_null() || !_frame_buffer.is_valid() )
    {
        return;
    }

    _textureWidth = width;
    _textureHeight = height;
    setFrameBuffer();

    // if ( _usedWidth != width || _usedHeight != height ) {
    //     _logger->info( "Video::refresh :: Resizing frame buffer to %u x %u", width, height );

    //     auto created_frame_buffer =
    //         godot::Image::create( width, height, false, _frame_buffer->get_format() );
    //     if ( created_frame_buffer.is_null() || !created_frame_buffer.is_valid() )
    //     {
    //         _logger->error( "Video::refresh :: Failed to recreate frame buffer" );
    //         return;
    //     }
    //     _frame_buffer = created_frame_buffer;
    // }

    switch ( _frame_buffer->get_format() )
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
    buffer_size = width * height * channels;
    _intermediary_buffer.resize( buffer_size );

    if(_rotation > 0) {
        rotateImage((unsigned char*)data, (unsigned char*)_intermediary_buffer.ptr(), width, height, channels, 4 - _rotation);
    }
    else {
        memcpy( (void *)_intermediary_buffer.ptr(), data, buffer_size );
    }
}

// Rotate raw image data (RGBA or any channel count)
void Video::rotateImage(
    const unsigned char* source_buffer, 
    unsigned char* dest_buffer,
    unsigned int width,
    unsigned int height,
    unsigned int channels,
    unsigned int rotation
) {
    int newWidth = (rotation == ROTATE_90 || rotation == ROTATE_270) ? height : width;
    int newHeight = (rotation == ROTATE_90 || rotation == ROTATE_270) ? width : height;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Source pixel index
            int srcIndex = (y * width + x) * channels;

            int dstX, dstY;
            switch (rotation) {
                case ROTATE_90: // 90 degrees
                    dstX = height - 1 - y;
                    dstY = x;
                    break;
                case ROTATE_180: // 180 degrees
                    dstX = width - 1 - x;
                    dstY = height - 1 - y;
                    break;
                case ROTATE_270: // 270 degrees
                    dstX = y;
                    dstY = width - 1 - x;
                    break;
            }

            // Destination pixel index
            int dstIndex = (dstY * newWidth + dstX) * channels;

            // Copy pixel (all channels)
            for (int c = 0; c < channels; ++c) {
                dest_buffer[dstIndex + c] = source_buffer[srcIndex + c];
            }
        }
    }

    return;
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
