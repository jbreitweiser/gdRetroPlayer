#pragma once

#include <godot_cpp/classes/mesh_instance3d.hpp>

#include "TextureRectRenderer.hpp"
#include "BaseMaterial3DRenderer.hpp"

using namespace godot;

class RenderSurfaceFactory {
public:
    RenderSurfaceFactory(lrcpp::Logger* logger) {
        _logger = logger;
    }
    ~RenderSurfaceFactory() {
        _logger = nullptr;
    }
    RenderSurface* createRenderer(Node* node);
private:
    lrcpp::Logger* _logger = nullptr;
};
