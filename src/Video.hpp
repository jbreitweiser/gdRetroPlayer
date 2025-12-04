#pragma once

#include "Config.hpp"
#include "render_surfaces/RenderSurfaceFactory.hpp"

#include <vector>
#include <cstdint>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <lrcpp/Components.h>

enum VideoRotation {
        ROTATE_0,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270,
        ROTATE_MAX
    };


class Video : public lrcpp::Video {
public:
    Video();
    ~Video();
    bool init(Config* config, lrcpp::Logger* logger);
    void destroy();

    double getCoreFps() const;

    void clear();
    void present();
    void set_texture_rect(godot::TextureRect *rect);
    void set_render_surface(godot::Node *node);

    // lrcpp::Video
    virtual bool setRotation(unsigned rotation) override;
    virtual bool getOverscan(bool* overscan) override;
    virtual bool getCanDupe(bool* canDupe) override;
    virtual bool showMessage(retro_message const* message) override;
    virtual bool setPixelFormat(retro_pixel_format format) override;
    virtual bool setHwRender(retro_hw_render_callback* callback) override;
    virtual bool setFrameTimeCallback(retro_frame_time_callback const* callback) override;
    virtual bool setSystemAvInfo(retro_system_av_info const* info) override;
    virtual bool setGeometry(retro_game_geometry const* geometry) override;
    virtual bool getCurrentSoftwareFramebuffer(retro_framebuffer* framebuffer) override;
    virtual bool getHwRenderInterface(retro_hw_render_interface const** interface) override;
    virtual bool setHwRenderContextNegotiationInterface(retro_hw_render_context_negotiation_interface const* interface) override;
    virtual bool setHwSharedContext() override;
    virtual bool getTargetRefreshRate(float* rate) override;
    virtual bool getPreferredHwRender(unsigned* preferred) override;

    virtual void refresh(void const* data, unsigned width, unsigned height, size_t pitch) override;

    virtual uintptr_t getCurrentFramebuffer() override;
    virtual retro_proc_address_t getProcAddress(char const* symbol) override;

protected:
    void rotateImage(const unsigned char* source_buffer, unsigned char* dest_buffer, unsigned int width, unsigned int height, unsigned int channels, unsigned int rotation);
    void reset();
    int frame_count = 0;
    lrcpp::Logger* _logger;

    unsigned _rotation = 0;  // requested rotation
    unsigned _lastRotation = _rotation; // last applied rotation
    float _aspectRatio;  // aspect ratio
    unsigned _textureWidth;  // requested width
    unsigned _textureHeight;  // requested height
    unsigned _lastTextureWidth;  // last width
    unsigned _lastTextureHeight;  // last height

    retro_pixel_format _pixelFormat;
    double _coreFps;
    
    godot::PackedByteArray _intermediary_buffer;
    RenderSurface* _renderer = nullptr;
};
