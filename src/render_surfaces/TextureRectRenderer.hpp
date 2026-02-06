#pragma once

#include "RenderSurface.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/image_texture.hpp>

class TextureRectRenderer : RenderSurface {
public:
    TextureRectRenderer();
    ~TextureRectRenderer();

    bool init(lrcpp::Logger* logger) override;
    void destroy() override;
    void present(godot::PackedByteArray* intermediary_buffer, unsigned width, unsigned height, unsigned rotation) override; 
    bool setPixelFormat(godot::Image::Format format) override;
    bool setFrameBuffer(unsigned usedWidth, unsigned usedHeight) override;
    godot::Image::Format get_format() override; 

    void setTextureRect(godot::TextureRect *rect);

private:
    godot::TextureRect* _texture_rect = nullptr;
    godot::Ref<godot::ImageTexture> _image_texture;
    godot::Ref<godot::Image> _frame_buffer;
    godot::Image::Format _textureFormat = godot::Image::Format::FORMAT_RGB565;  // set to default
    unsigned int _channels;
    unsigned _bufferWidth;
    unsigned _bufferHeight;
    lrcpp::Logger* _logger = nullptr;
};
