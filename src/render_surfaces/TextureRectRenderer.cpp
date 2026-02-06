#include "TextureRectRenderer.hpp"
#include <cstring>

TextureRectRenderer::TextureRectRenderer() {};

TextureRectRenderer::~TextureRectRenderer() {};

bool TextureRectRenderer::init(lrcpp::Logger* logger) {
   _logger = logger;
    return true;
}

void TextureRectRenderer::destroy() {
    _frame_buffer.unref();
    _image_texture.unref();
    _bufferWidth = _bufferHeight = 0;
}

void TextureRectRenderer::setTextureRect(godot::TextureRect *rect) {
    _texture_rect = rect;
    // If we already have an image texture, assign it immediately
    if (_image_texture.is_valid() && _texture_rect != nullptr) {
        _texture_rect->set_texture(_image_texture);
    }
}

//  Reset framebuffer definition if there is a change in size
bool TextureRectRenderer::setFrameBuffer(unsigned bufferWidth, unsigned bufferHeight) {
    // If we already have a frame buffer, check if size matches
    if (_frame_buffer != nullptr) {
        if (bufferWidth == _bufferWidth && bufferHeight == _bufferHeight) {
            return true;
        }

        // Invalidate framebuffer and ImageTexture so it will be recreated for the new size
        _frame_buffer.unref();
        _image_texture.unref();
    }
    
    _bufferWidth = bufferWidth;
    _bufferHeight = bufferHeight;

    _frame_buffer = godot::Image::create( _bufferWidth, _bufferHeight, false, _textureFormat );
    
    if (_frame_buffer == nullptr) {
        _logger->error("[Video::setGeometry] _frame_buffer failed: to create");
        return false;
    }

    return true;
}

bool TextureRectRenderer::setPixelFormat(godot::Image::Format format) {
    _textureFormat = format;

    switch (_textureFormat) {
        case godot::Image::Format::FORMAT_RGB565:
            _channels = 2;
            return _channels;
        case godot::Image::Format::FORMAT_RGBA8:
             _channels = 4;
            return _channels;
        default: 
            _logger->error("TextureRectRenderer::setPixelFormat :: Unhandled pixel format: %d", format);
            _channels = 0;
            return 0;
    }
}

godot::Image::Format TextureRectRenderer::get_format() {
    return _textureFormat;
}

// Copy data from intermediary buffer to Image buffer and update TextureRect
void TextureRectRenderer::present(godot::PackedByteArray* intermediary_buffer, 
                            unsigned width, 
                            unsigned height, 
                            unsigned rotation) {
    if (_frame_buffer.is_null() || !_frame_buffer.is_valid()) {
        return;
    }

    // Update internal Image with the latest pixel data
    _frame_buffer->set_data(width, height, false, _frame_buffer->get_format(), *intermediary_buffer);

    // Apply rotation if needed
    switch (rotation) {
        case 1: // 90 degrees
            _frame_buffer->rotate_90(godot::COUNTERCLOCKWISE);
            break;
        case 2: // 180 degrees
            _frame_buffer->rotate_180();
            break;
        case 3: // 270 degrees
            _frame_buffer->rotate_90(godot::CLOCKWISE);
            break;
    }

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

