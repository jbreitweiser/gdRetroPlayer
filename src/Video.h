#pragma once

#include "Config.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <lrcpp/Components.h>

class Video : public lrcpp::Video {
public:
    Video();

    bool init(Config* config, lrcpp::Logger* logger);
    void destroy();

    double getCoreFps() const;

    void clear();
    void present();
    void set_texture_rect(godot::TextureRect *rect);
    godot::Ref<godot::Image> get_frame_buffer()
    {
        return _frame_buffer;
    }

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
    void reset();
    int frame_count = 0;
    lrcpp::Logger* _logger;

    godot::Ref<godot::Image> _frame_buffer;

    //SDL_Window* _window;
    //SDL_Renderer* _renderer;

    retro_pixel_format _pixelFormat;
    double _coreFps;
    float _aspectRatio;
    godot::PackedByteArray _intermediary_buffer;
    godot::TextureRect* _texture_rect;
    godot::Ref<godot::ImageTexture> _image_texture;
    //SDL_Texture* _texture;
    unsigned _textureWidth;
    unsigned _textureHeight;
    godot::Image::Format _textureFormat;
    unsigned _usedWidth;
    unsigned _usedHeight;
};
