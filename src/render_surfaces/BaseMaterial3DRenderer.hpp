#pragma once

#include "RenderSurface.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/base_material3d.hpp>

class BaseMaterial3DRenderer : RenderSurface {
public:
    BaseMaterial3DRenderer();
    ~BaseMaterial3DRenderer();

    bool init(lrcpp::Logger* logger) override;
    void destroy() override;
    void present(godot::PackedByteArray* intermediary_buffer, unsigned width, unsigned height, unsigned rotation) override; 
    bool setPixelFormat(godot::Image::Format format) override;
    bool setFrameBuffer(unsigned usedWidth, unsigned usedHeight) override;
    godot::Image::Format get_format() override; 

    void setBaseMaterial3D(godot::BaseMaterial3D *material);

private:
    godot::BaseMaterial3D* _surface = nullptr;
    godot::Ref<godot::ImageTexture> _image_texture;
    godot::Ref<godot::Image> _frame_buffer;
    godot::Image::Format _textureFormat = godot::Image::Format::FORMAT_RGB565;  // set to default
    unsigned int _channels;
    unsigned _usedWidth;
    unsigned _usedHeight;
    lrcpp::Logger* _logger = nullptr;
};
