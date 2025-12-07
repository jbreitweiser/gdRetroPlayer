#include "RenderSurfaceFactory.hpp"
#include "TextureRectRenderer.hpp"
#include "BaseMaterial3DRenderer.hpp"

RenderSurface* RenderSurfaceFactory::createRenderer(Node* node) {

    if (node == nullptr) {
        return nullptr;
    }

    if (MeshInstance3D* mesh = Object::cast_to<MeshInstance3D>(node)) {
        Material *mat = mesh->get_surface_override_material(0).ptr();
        if (mat == nullptr) {
            _logger->error("MeshInstance3D has no active material.");
            return nullptr;
        }

        if(BaseMaterial3D* mat3d = Object::cast_to<BaseMaterial3D>(mat)){
            BaseMaterial3DRenderer* renderer = new BaseMaterial3DRenderer();
            renderer->setBaseMaterial3D(mat3d);
            renderer->init(_logger);
            return (RenderSurface*)renderer;
        } else {
            _logger->error("MeshInstance3D material is not a BaseMaterial3D.");
            return nullptr;
        }
    } else if (TextureRect* rect = Object::cast_to<TextureRect>(node)) {
        TextureRectRenderer* renderer = new TextureRectRenderer();
        renderer->setTextureRect(rect);
        renderer->init(_logger);
        return (RenderSurface*)renderer;
    } else {
        // Handle other types or the base Object type
        _logger->error("Object is of an unhandled type.");
        return nullptr;
    }
}

