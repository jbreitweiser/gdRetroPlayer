#pragma once

#include <lrcpp/Components.h>

#include <godot_cpp/classes/image.hpp>

class AudioPlayer {
public:
    virtual void present(godot::PackedVector2Array* samples) = 0; 
};
