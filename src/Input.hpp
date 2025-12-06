#pragma once

#include <lrcpp/Components.h>

#include <string>
#include <map>
#include <vector>

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/core/print_string.hpp>

class RetroInput: public lrcpp::Input {
public:
    RetroInput();

    bool init(lrcpp::Logger* logger);
    void destroy();

    void process(godot::InputEvent* event);
    void process( const godot::Dictionary &event );

    // lrcpp::Input
    virtual bool setInputDescriptors(retro_input_descriptor const* descriptors) override;
    virtual bool setKeyboardCallback(retro_keyboard_callback const* callback) override;
    virtual bool getInputDeviceCapabilities(uint64_t* capabilities) override;
    virtual bool setControllerInfo(retro_controller_info const* info) override;
    virtual bool getInputBitmasks(bool* supports) override;

    virtual int16_t state(unsigned port, unsigned device, unsigned index, unsigned id) override;
    virtual void poll() override;

protected:
    void process_gamepad(int32_t device_id, int button_index, bool pressed);
    void init_joypads(int32_t device_id, std::string joypad_name);
    void process(godot::InputEventKey const* event);
    void process(godot::InputEventMouseButton const* btn_event);
    void process(godot::InputEventMouseMotion const* motion_event);

    void reset();

    static unsigned keycodeToLibretro(godot::Key code, godot::KeyLocation location);

    godot::Dictionary controller_info;

    struct Gamepad {
        Gamepad();

        struct Axes {
            int16_t x, y;
        };

        int32_t deviceIndex;
        std::string controllerName;
        int lastDir[6];
        bool state[16];
        Axes analogs[3];
        float sensitivity;
        bool digital;
    };

    lrcpp::Logger* _logger;

    std::map<int32_t, Gamepad> _gamepads;
    std::vector<Gamepad*> _ports;

    int _mouseX;
    int _mouseY;
    bool _mouseButtons[5];

    retro_keyboard_callback _keyboardCallback;
    bool _keyboardPreviousState[RETROK_LAST];
    bool _keyboardState[RETROK_LAST];
};
