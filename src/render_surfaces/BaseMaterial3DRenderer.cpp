#include "BaseMaterial3DRenderer.hpp"

BaseMaterial3DRenderer::BaseMaterial3DRenderer() {};

BaseMaterial3DRenderer::~BaseMaterial3DRenderer() {};

bool BaseMaterial3DRenderer::init(lrcpp::Logger* logger) {
   _logger = logger;
    return true;
}

void BaseMaterial3DRenderer::destroy() {
    _frame_buffer.unref();
    _image_texture.unref();
    _usedWidth = _usedHeight = 0;
}

void BaseMaterial3DRenderer::setBaseMaterial3D(godot::BaseMaterial3D *surface) {
    _surface = surface;
    // If we already have an image texture, assign it immediately
    if (_image_texture.is_valid() && _surface != nullptr) {
        _surface->set_texture( godot::BaseMaterial3D::TEXTURE_ALBEDO,_image_texture);
    }
}

//  Reset framebuffer definition if there is a change in size
bool BaseMaterial3DRenderer::setFrameBuffer(unsigned usedWidth, unsigned usedHeight) {
    // If we already have a frame buffer, check if size matches
    if (_frame_buffer != nullptr) {
        if (usedWidth == _usedWidth && _usedHeight == usedHeight) {
            return true;
        }

        // Invalidate framebuffer and ImageTexture so it will be recreated for the new size
        _frame_buffer.unref();
        _image_texture.unref();
    }
    
    _frame_buffer = godot::Image::create( usedWidth, usedHeight, false, _textureFormat );
    // _intermediary_buffer.resize( _usedWidth * _usedHeight * _channels );

    if (_frame_buffer == nullptr) {
        _logger->error("[Video::setGeometry] _frame_buffer failed: to create");
        return false;
    }

    _usedWidth = usedWidth;
    _usedHeight = usedHeight;

    return true;
}

bool BaseMaterial3DRenderer::setPixelFormat(godot::Image::Format format) {
    _textureFormat = format;

    switch (_textureFormat) {
        case godot::Image::Format::FORMAT_RGB565:
            _channels = 2;
            return _channels;
        case godot::Image::Format::FORMAT_RGBA8:
             _channels = 4;
            return _channels;
        default: 
            _logger->error("BaseMaterial3DRenderer::setPixelFormat :: Unhandled pixel format: %d", format);
            _channels = 0;
            return 0;
    }
}

godot::Image::Format BaseMaterial3DRenderer::get_format() {
    return _textureFormat;
}

// Copy data from intermediary buffer to Image buffer and update TextureRect
void BaseMaterial3DRenderer::present(godot::PackedByteArray* intermediary_buffer) {
    if (_frame_buffer.is_null() || !_frame_buffer.is_valid()) {
        return;
    }

    // Update internal Image with the latest pixel data
    _frame_buffer->set_data(_frame_buffer->get_width(), _frame_buffer->get_height(), false, _frame_buffer->get_format(), *intermediary_buffer);

    // Ensure we have an ImageTexture to update
    if (_image_texture.is_null() || !_image_texture.is_valid()) {

        _image_texture = godot::ImageTexture::create_from_image(_frame_buffer);


        if (_image_texture.is_null() || !_image_texture.is_valid()) {
            _logger->error("Video::present :: Failed to create ImageTexture");
            return;
        }

        // Assign texture to TextureRect if available
        if (_surface != nullptr) {
            _surface->set_texture( godot::BaseMaterial3D::TEXTURE_ALBEDO,_image_texture);
        }
    }
    else {
        // Update existing texture with new image data (more efficient)
        _image_texture->update(_frame_buffer);
    }
}

