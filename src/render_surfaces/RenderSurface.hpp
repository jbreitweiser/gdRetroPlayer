#pragma once

#include <lrcpp/Components.h>

#include <godot_cpp/classes/image.hpp>

class RenderSurface {
public:
    virtual bool init(lrcpp::Logger* logger) = 0;
    virtual void destroy() = 0;
    virtual void present(godot::PackedByteArray* intermediary_buffer, unsigned width, unsigned height, unsigned rotation) = 0; 
    virtual bool setPixelFormat(godot::Image::Format format) = 0;
    virtual bool setFrameBuffer(unsigned usedWidth, unsigned usedHeight) = 0;
    virtual godot::Image::Format get_format() = 0; 
};
