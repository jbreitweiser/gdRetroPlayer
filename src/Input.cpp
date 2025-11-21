#include "Input.h"

RetroInput::RetroInput() {
    reset();
}

bool RetroInput::init(lrcpp::Logger* logger) {
    reset();

    _logger = logger;
    init_joypads();
    _logger->info("Input subsystem Not implemented");
    return true;
}

void RetroInput::destroy() {
    reset();
}

void RetroInput::process( const godot::Dictionary &event ){

    godot::String event_type = (godot::String)event["event"];

    _logger->info("Setting input descriptors");

    if(event_type == "InputEventJoypadButton"){
        int32_t device_id = (int32_t)event["device"];
        godot::JoyButton button_index = (godot::JoyButton)(int)event["button_index"];
        bool pressed = (bool)event["pressed"];
        process_gamepad(device_id, button_index, pressed);
        return;
    }

    if(event_type == "InitializeControllerEvent") {
        int32_t device_id = (int32_t)event["device"];
        godot::String joypad_name = event["joypad_name"];
        init_joypads(device_id, joypad_name.utf8().get_data());
        return;
    }

}

void RetroInput::process(godot::InputEvent *event) {
    if (event->is_class("InputEventKey")) {
        godot::InputEventKey *key_event = static_cast<godot::InputEventKey *>(event);
        process(key_event);
        return;
    }

    if (event->is_class("InputEventMouseButton")) {
        godot::InputEventMouseButton *btn_event = static_cast<godot::InputEventMouseButton *>(event);
        process(btn_event);
        return;
    }

    if (event->is_class("InputEventMouseMotion")) {
        godot::InputEventMouseMotion *motion_event = static_cast<godot::InputEventMouseMotion *>(event);
        process(motion_event);
        return;
    }

    if (event->is_class("InputEventAction")) {
        godot::InputEventAction *action_event = static_cast<godot::InputEventAction *>(event);
        _logger->warn("InputEventAction %s not implemented", action_event->get_action());
        //process(action_event);
        return;
    }

    //  if (event->is_class("InputEventJoypadButton")) {
    //     godot::InputEventJoypadButton *action_event = static_cast<godot::InputEventJoypadButton *>(event);
    //     process(action_event);
    //     return;
    // }

    /*  TODO implement other input types
    InputEventScreenTouch
    InputEventJoypadButton
    InputEventJoypadMotion
    */

}

bool RetroInput::setInputDescriptors(retro_input_descriptor const* descriptors) {
    _logger->info("Setting input descriptors");
    _logger->info("    port device index id description");

    for (size_t i = 0; descriptors[i].description != nullptr; i++) {
        /**
         * At least the Frodo core doesn't properly terminate the input
         * descriptor list with a zeroed entry, we do our best to avoid a crash
         * here.
         */
        if ((descriptors[i].device & RETRO_DEVICE_MASK) > RETRO_DEVICE_POINTER) {
            break;
        }

        if (descriptors[i].id > RETRO_DEVICE_ID_LIGHTGUN_RELOAD) {
            break;
        }

        retro_input_descriptor const* desc = descriptors + i;
        _logger->info("    %4u %6u %5u %2u %s", desc->port, desc->device, desc->index, desc->id, desc->description);
    }

    return true;
}

// RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS
bool RetroInput::setKeyboardCallback(retro_keyboard_callback const* callback) {
    _keyboardCallback = *callback;
    return true;
}

// RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK
bool RetroInput::getInputDeviceCapabilities(uint64_t* capabilities) {
    *capabilities = 1 << RETRO_DEVICE_JOYPAD | 1 << RETRO_DEVICE_MOUSE | 1 << RETRO_DEVICE_KEYBOARD | 1 << RETRO_DEVICE_ANALOG;
    return true;
}

// RETRO_ENVIRONMENT_SET_CONTROLLER_INFO
// need to record the available controllers and their types
// so we can respond to state() calls correctly
bool RetroInput::setControllerInfo(retro_controller_info const* info) {
    static char const* const deviceNames[] = {"none", "joypad", "mouse", "keyboard", "lightgun", "analog", "pointer"};

    _logger->info("Setting controller info");
    _logger->info("    port id type     description");

    for (size_t i = 0; info[i].types != nullptr; i++) {
        for (unsigned j = 0; j < info[i].num_types; j++) {
            retro_controller_description const* type = info[i].types + j;

            unsigned const deviceType = type->id & RETRO_DEVICE_MASK;
            char const* deviceName = deviceType < sizeof(deviceNames) / sizeof(deviceNames[0]) ? deviceNames[deviceType] : "?";

            _logger->info("    %4zu %2u %-8s %s", i + 1, type->id >> RETRO_DEVICE_TYPE_SHIFT, deviceName, type->desc);
        }
    }

    return true;
}

bool RetroInput::getInputBitmasks(bool* supports) {
    *supports = false;
    return false;
}

//  callback from the core to get state of the game pads
int16_t RetroInput::state(unsigned port, unsigned device, unsigned index, unsigned id) {
    unsigned const base = device & RETRO_DEVICE_MASK;

    switch (base) {
        case RETRO_DEVICE_JOYPAD:
        case RETRO_DEVICE_ANALOG: {
            if (port >= _ports.size()) {
                return 0;
            }

            Gamepad* const gamepad = _ports[port];

            if (base == RETRO_DEVICE_JOYPAD) {
                return gamepad->state[id];
            }
            else {
                return id == RETRO_DEVICE_ID_ANALOG_X ? gamepad->analogs[index].x : gamepad->analogs[index].y;
            }

            break;
        }

        case RETRO_DEVICE_KEYBOARD: return _keyboardState[id] ? 32767 : 0;

        case RETRO_DEVICE_MOUSE: {
            switch (id) {
                case RETRO_DEVICE_ID_MOUSE_X: return _mouseX;
                case RETRO_DEVICE_ID_MOUSE_Y: return _mouseY;
                case RETRO_DEVICE_ID_MOUSE_LEFT: return _mouseButtons[0] ? 32767 : 0;
                case RETRO_DEVICE_ID_MOUSE_MIDDLE: return _mouseButtons[1] ? 32767 : 0;
                case RETRO_DEVICE_ID_MOUSE_RIGHT: return _mouseButtons[2] ? 32767 : 0;
                case RETRO_DEVICE_ID_MOUSE_BUTTON_4: return _mouseButtons[3] ? 32767 : 0;
                case RETRO_DEVICE_ID_MOUSE_BUTTON_5: return _mouseButtons[4] ? 32767 : 0;
            }

            break;
        }
    }

    return 0;
}

//  Callback from the core to get the state of the keyboard
void RetroInput::poll() {
    if (_keyboardCallback.callback != nullptr) {
        for (int i = RETROK_FIRST; i < RETROK_LAST; i++) {
            if (_keyboardState[i] != _keyboardPreviousState[i]) {
                _keyboardCallback.callback(_keyboardState[i], i, 0, 0);
                _keyboardPreviousState[i] = _keyboardState[i];
            }
        }
    }
}


//  need to have a change process for detecting gamepads being added
// void RetroInput::process(SDL_JoyDeviceEvent const* event) {
//     if (event->type != SDL_JOYDEVICEADDED) {
//         return;
//     }

//     SDL_JoystickGUID const guid = SDL_JoystickGetDeviceGUID(event->which);
//     char const* const mapping = SDL_GameControllerMappingForGUID(guid);
//     char const* const name = SDL_JoystickNameForIndex(event->which);

//     if (mapping == nullptr) {
//         char guidStr[128];
//         SDL_JoystickGetGUIDString(guid, guidStr, sizeof(guidStr));
//         _logger->error("No mapping for joystick \"%s\" (GUID %s), joystick unusable", name, guidStr);
//     }
//     else {
//         SDL_free((void*)mapping);
//     }
// }

void RetroInput::init_joypads() {
    // Get the Input singleton instance
    godot::Input* input = godot::Input::get_singleton();
    
    // Get the list of connected joypad device IDs
    // The device IDs are typically integers (e.g., 0, 1, 2, ...)
    godot::TypedArray<int32_t> joypad_devices = input->get_connected_joypads();

    godot::UtilityFunctions::print("Connected Joypads Count: ", joypad_devices.size());

    for (int i = 0; i < joypad_devices.size(); ++i) {
        int device_id = joypad_devices[i];
        
        // Get the name of the joypad
        godot::String name = input->get_joy_name(device_id);
        
        // Get the GUID of the joypad (useful for mapping)
        godot::String guid = input->get_joy_guid(device_id);
        
        // Get platform-specific information (optional)
        godot::Dictionary info = input->get_joy_info(device_id);

        godot::print_line("Device ID: " + godot::String::num(device_id));
        godot::print_line("  Name: " + name);
        godot::print_line("  GUID: " + guid);
        
        if (!info.is_empty()) {
            godot::print_line("  Info: ", info);
        }

        init_joypads(device_id, name.utf8().get_data());
    }
}

void RetroInput::init_joypads(int32_t device_id, std::string joypad_name) {
    
        auto inserted = _gamepads.insert(std::make_pair(device_id, Gamepad()));
        Gamepad* const gamepad = &inserted.first->second;

        gamepad->deviceIndex = device_id;
        gamepad->controllerName = joypad_name;
        gamepad->joystickName = joypad_name;

        _ports.emplace_back(gamepad);
        _logger->info("Controller %s (%s) added", gamepad->controllerName.c_str(), gamepad->joystickName.c_str());
}


// void RetroInput::process(SDL_ControllerDeviceEvent const* event) {
//     if (!SDL_IsGameController(event->which)) {
//         _logger->warn("SDL device %d is not a controller", event->which);
//         return;
//     }

//     if (event->type == SDL_CONTROLLERDEVICEADDED) {
//         SDL_GameController* const controller = SDL_GameControllerOpen(event->which);

//         if (controller == nullptr) {
//             _logger->error("SDL_GameControllerOpen() failed: %s", SDL_GetError());
//             return;
//         }

//         SDL_Joystick* const joystick = SDL_GameControllerGetJoystick(controller);

//         if (joystick == nullptr) {
//             _logger->error("SDL_GameControllerGetJoystick() failed: %s", SDL_GetError());
//             SDL_GameControllerClose(controller);
//             return;
//         }

//         auto inserted = _gamepads.insert(std::make_pair(event->which, Gamepad()));
//         Gamepad* const gamepad = &inserted.first->second;

//         gamepad->controller = controller;
//         gamepad->joystick = joystick;
//         gamepad->deviceIndex = event->which;
//         gamepad->instanceId = SDL_JoystickInstanceID(joystick);
//         gamepad->controllerName = SDL_GameControllerName(controller);
//         gamepad->joystickName = SDL_JoystickName(joystick);

//         _ports.emplace_back(gamepad);
//         _logger->info("Controller %s (%s) added", gamepad->controllerName.c_str(), gamepad->joystickName.c_str());

//         size_t const count = _ports.size();

//         for (size_t i = 0; i < count; i++) {
//             Gamepad const* const gamepad = _ports[i];
//             _logger->info("    Port %zu has controller %s (%s)", i + 1, gamepad->controllerName.c_str(), gamepad->joystickName.c_str());
//         }
//     }
//     else if (event->type == SDL_CONTROLLERDEVICEREMOVED) {
//         auto found = _gamepads.find(event->which);

//         if (found == _gamepads.end()) {
//             return;
//         }

//         Gamepad* const gamepad = &found->second;
//         SDL_GameControllerClose(gamepad->controller);

//         for (auto it = _ports.begin(); it != _ports.end(); ++it) {
//             if (*it == gamepad) {
//                 _ports.erase(it);
//                 break;
//             }
//         }

//         _gamepads.erase(found);
//         _logger->info("Controller %s (%s) removed", gamepad->controllerName.c_str(), gamepad->joystickName.c_str());

//         size_t const count = _ports.size();

//         for (size_t i = 0; i < count; i++) {
//             Gamepad const* const gamepad = _ports[i];
//             _logger->info("    Port %zu has controller %s (%s)", i + 1, gamepad->controllerName.c_str(), gamepad->joystickName.c_str());
//         }
//     }
// }


void RetroInput::process_gamepad(int32_t device_id, godot::JoyButton button_index, bool pressed) {
    //  
    auto found = _gamepads.find(device_id);
_logger->info("Game Pad ID %d button %d", device_id, button_index);
    if (found == _gamepads.end()) {
        _logger->info("Gamepad not present");
        return;
    }

    Gamepad* const gamepad = &found->second;
    unsigned button = 0;

    switch (button_index) {
        case godot::JoyButton::JOY_BUTTON_A: button = RETRO_DEVICE_ID_JOYPAD_B; break;
        case godot::JoyButton::JOY_BUTTON_B: button = RETRO_DEVICE_ID_JOYPAD_A; break;
        case godot::JoyButton::JOY_BUTTON_X: button = RETRO_DEVICE_ID_JOYPAD_Y; break;
        case godot::JoyButton::JOY_BUTTON_Y: button = RETRO_DEVICE_ID_JOYPAD_X; break;
        case godot::JoyButton::JOY_BUTTON_BACK: button = RETRO_DEVICE_ID_JOYPAD_SELECT; break;
        case godot::JoyButton::JOY_BUTTON_START: button = RETRO_DEVICE_ID_JOYPAD_START; break;
        case godot::JoyButton::JOY_BUTTON_LEFT_STICK: button = RETRO_DEVICE_ID_JOYPAD_L3; break;
        case godot::JoyButton::JOY_BUTTON_RIGHT_STICK: button = RETRO_DEVICE_ID_JOYPAD_R3; break;
        case godot::JoyButton::JOY_BUTTON_LEFT_SHOULDER: button = RETRO_DEVICE_ID_JOYPAD_L; break;
        case godot::JoyButton::JOY_BUTTON_RIGHT_SHOULDER: button = RETRO_DEVICE_ID_JOYPAD_R; break;
        case godot::JoyButton::JOY_BUTTON_DPAD_UP: button = RETRO_DEVICE_ID_JOYPAD_UP; break;
        case godot::JoyButton::JOY_BUTTON_DPAD_DOWN: button = RETRO_DEVICE_ID_JOYPAD_DOWN; break;
        case godot::JoyButton::JOY_BUTTON_DPAD_LEFT: button = RETRO_DEVICE_ID_JOYPAD_LEFT; break;
        case godot::JoyButton::JOY_BUTTON_DPAD_RIGHT: button = RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
        case godot::JoyButton::JOY_BUTTON_PADDLE1: button = RETRO_DEVICE_ID_JOYPAD_A; break;
        case godot::JoyButton::JOY_BUTTON_PADDLE2: button = RETRO_DEVICE_ID_JOYPAD_B; break;
        case godot::JoyButton::JOY_BUTTON_PADDLE3: button = RETRO_DEVICE_ID_JOYPAD_X; break;
        case godot::JoyButton::JOY_BUTTON_PADDLE4: button = RETRO_DEVICE_ID_JOYPAD_Y; break;
        case godot::JoyButton::JOY_BUTTON_GUIDE: // fallthrough
        default: return;
    }
_logger->info("RETRO_DEVICE_ID_JOYPAD button %d", button);
    gamepad->state[button] = pressed;
}

// void RetroInput::process(SDL_ControllerAxisEvent const* event) {
//     auto found = _gamepads.find(event->which);

//     if (found == _gamepads.end()) {
//         return;
//     }

//     Gamepad* const gamepad = &found->second;

//     int const threshold = 32767 * (1.0f - gamepad->sensitivity);
//     int positive = 0, negative = 0;
//     int button = 0;
//     int* lastDir = nullptr;

//     switch (event->axis) {
//         case SDL_CONTROLLER_AXIS_LEFTX:
//         case SDL_CONTROLLER_AXIS_LEFTY:
//         case SDL_CONTROLLER_AXIS_RIGHTX:
//         case SDL_CONTROLLER_AXIS_RIGHTY:
//             switch (event->axis) {
//                 case SDL_CONTROLLER_AXIS_LEFTX:
//                     gamepad->analogs[RETRO_DEVICE_INDEX_ANALOG_LEFT].x = event->value;
//                     positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
//                     negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
//                     lastDir = gamepad->lastDir + 0;
//                     break;

//                 case SDL_CONTROLLER_AXIS_LEFTY:
//                     gamepad->analogs[RETRO_DEVICE_INDEX_ANALOG_LEFT].y = event->value;
//                     positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
//                     negative = RETRO_DEVICE_ID_JOYPAD_UP;
//                     lastDir = gamepad->lastDir + 1;
//                     break;

//                 case SDL_CONTROLLER_AXIS_RIGHTX:
//                     gamepad->analogs[RETRO_DEVICE_INDEX_ANALOG_RIGHT].x = event->value;
//                     positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
//                     negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
//                     lastDir = gamepad->lastDir + 2;
//                     break;

//                 case SDL_CONTROLLER_AXIS_RIGHTY:
//                     gamepad->analogs[RETRO_DEVICE_INDEX_ANALOG_RIGHT].y = event->value;
//                     positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
//                     negative = RETRO_DEVICE_ID_JOYPAD_UP;
//                     lastDir = gamepad->lastDir + 3;
//                     break;
//             }

//             if (event->value < -threshold) {
//                 button = negative;
//             }
//             else if (event->value > threshold) {
//                 button = positive;
//             }
//             else {
//                 button = -1;
//             }

//             break;

//         case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
//         case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
//             if (event->axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
//                 gamepad->analogs[RETRO_DEVICE_INDEX_ANALOG_BUTTON].x = event->value;
//                 button = RETRO_DEVICE_ID_JOYPAD_L2;
//                 lastDir = gamepad->lastDir + 4;
//             }
//             else {
//                 gamepad->analogs[RETRO_DEVICE_INDEX_ANALOG_BUTTON].y = event->value;
//                 button = RETRO_DEVICE_ID_JOYPAD_R2;
//                 lastDir = gamepad->lastDir + 5;
//             }

//             break;

//         default:
//             return;
//     }

//     if (gamepad->digital) {
//         if (*lastDir != -1) {
//             gamepad->state[*lastDir] = false;
//         }

//         if (event->value < -threshold || event->value > threshold) {
//             gamepad->state[button] = true;
//         }

//         *lastDir = button;
//     }
// }

void RetroInput::process(godot::InputEventKey const* event) {
    unsigned const key = keycodeToLibretro(event->get_keycode(), event->get_location() );

    if (key != RETROK_UNKNOWN) {
        _keyboardState[key] = event->is_pressed();;
    }
}

void RetroInput::process(godot::InputEventMouseButton const* btn_event) {
    switch (btn_event->get_button_index()) {
        case godot::MouseButton::MOUSE_BUTTON_LEFT:
            _mouseButtons[0] = (btn_event->get_button_mask() & godot::MouseButtonMask::MOUSE_BUTTON_MASK_LEFT) != 0;
            break;

        case godot::MouseButton::MOUSE_BUTTON_MIDDLE:
            _mouseButtons[1] = (btn_event->get_button_mask() & godot::MouseButtonMask::MOUSE_BUTTON_MASK_MIDDLE) != 0;
            break;

        case godot::MouseButton::MOUSE_BUTTON_RIGHT:
            _mouseButtons[2] = (btn_event->get_button_mask() & godot::MouseButtonMask::MOUSE_BUTTON_MASK_RIGHT) != 0;
            break;

        case godot::MouseButton::MOUSE_BUTTON_XBUTTON1:
            _mouseButtons[3] = (btn_event->get_button_mask() & godot::MouseButtonMask::MOUSE_BUTTON_MASK_MB_XBUTTON1) != 0;
            break;

        case godot::MouseButton::MOUSE_BUTTON_XBUTTON2:
            _mouseButtons[4] = (btn_event->get_button_mask() & godot::MouseButtonMask::MOUSE_BUTTON_MASK_MB_XBUTTON2) != 0;
            break;
    }
}

void RetroInput::process(godot::InputEventMouseMotion const* motion_event) {
    godot::Vector2 mouse_vector = motion_event->get_relative();
    _mouseX += mouse_vector.x;
    _mouseY += mouse_vector.y;
}

void RetroInput::reset() {
    _logger = nullptr;

    // _gamepads.clear();
    // _ports.clear();

    _mouseX = _mouseY = 0;
    memset(_mouseButtons, 0, sizeof(_mouseButtons));

    _keyboardCallback.callback = nullptr;
    memset(_keyboardPreviousState, 0, sizeof(_keyboardPreviousState));
    memset(_keyboardState, 0, sizeof(_keyboardState));
}

unsigned RetroInput::keycodeToLibretro(godot::Key code, godot::KeyLocation location) {
    switch (code) {
        case godot::Key::KEY_ENTER: return RETROK_RETURN;
        case godot::Key::KEY_ESCAPE: return RETROK_ESCAPE;
        case godot::Key::KEY_BACKSPACE: return RETROK_BACKSPACE;
        case godot::Key::KEY_TAB: return RETROK_TAB;
        case godot::Key::KEY_SPACE: return RETROK_SPACE;
        case godot::Key::KEY_EXCLAM: return RETROK_EXCLAIM;
        case godot::Key::KEY_QUOTEDBL: return RETROK_QUOTEDBL;
        case godot::Key::KEY_NUMBERSIGN: return RETROK_HASH;
        case godot::Key::KEY_DOLLAR: return RETROK_DOLLAR;
        case godot::Key::KEY_AMPERSAND: return RETROK_AMPERSAND;
        case godot::Key::KEY_APOSTROPHE: return RETROK_QUOTE;
        case godot::Key::KEY_PARENLEFT: return RETROK_LEFTPAREN;
        case godot::Key::KEY_PARENRIGHT: return RETROK_RIGHTPAREN;
        case godot::Key::KEY_ASTERISK: return RETROK_ASTERISK;
        case godot::Key::KEY_PLUS: return RETROK_PLUS;
        case godot::Key::KEY_COMMA: return RETROK_COMMA;
        case godot::Key::KEY_MINUS: return RETROK_MINUS;
        case godot::Key::KEY_PERIOD: return RETROK_PERIOD;
        case godot::Key::KEY_SLASH: return RETROK_SLASH;
        case godot::Key::KEY_0: return RETROK_0;
        case godot::Key::KEY_1: return RETROK_1;
        case godot::Key::KEY_2: return RETROK_2;
        case godot::Key::KEY_3: return RETROK_3;
        case godot::Key::KEY_4: return RETROK_4;
        case godot::Key::KEY_5: return RETROK_5;
        case godot::Key::KEY_6: return RETROK_6;
        case godot::Key::KEY_7: return RETROK_7;
        case godot::Key::KEY_8: return RETROK_8;
        case godot::Key::KEY_9: return RETROK_9;
        case godot::Key::KEY_COLON: return RETROK_COLON;
        case godot::Key::KEY_SEMICOLON: return RETROK_SEMICOLON;
        case godot::Key::KEY_LESS: return RETROK_LESS;
        case godot::Key::KEY_EQUAL: return RETROK_EQUALS;
        case godot::Key::KEY_GREATER: return RETROK_GREATER;
        case godot::Key::KEY_QUESTION: return RETROK_QUESTION;
        case godot::Key::KEY_AT: return RETROK_AT;
        case godot::Key::KEY_BRACKETLEFT: return RETROK_LEFTBRACKET;
        case godot::Key::KEY_BACKSLASH: return RETROK_BACKSLASH;
        case godot::Key::KEY_BRACKETRIGHT: return RETROK_RIGHTBRACKET;
        case godot::Key::KEY_ASCIICIRCUM: return RETROK_CARET;
        case godot::Key::KEY_UNDERSCORE: return RETROK_UNDERSCORE;
        case godot::Key::KEY_QUOTELEFT: return RETROK_BACKQUOTE;
        case godot::Key::KEY_A: return RETROK_a;
        case godot::Key::KEY_B: return RETROK_b;
        case godot::Key::KEY_C: return RETROK_c;
        case godot::Key::KEY_D: return RETROK_d;
        case godot::Key::KEY_E: return RETROK_e;
        case godot::Key::KEY_F: return RETROK_f;
        case godot::Key::KEY_G: return RETROK_g;
        case godot::Key::KEY_H: return RETROK_h;
        case godot::Key::KEY_I: return RETROK_i;
        case godot::Key::KEY_J: return RETROK_j;
        case godot::Key::KEY_K: return RETROK_k;
        case godot::Key::KEY_L: return RETROK_l;
        case godot::Key::KEY_M: return RETROK_m;
        case godot::Key::KEY_N: return RETROK_n;
        case godot::Key::KEY_O: return RETROK_o;
        case godot::Key::KEY_P: return RETROK_p;
        case godot::Key::KEY_Q: return RETROK_q;
        case godot::Key::KEY_R: return RETROK_r;
        case godot::Key::KEY_S: return RETROK_s;
        case godot::Key::KEY_T: return RETROK_t;
        case godot::Key::KEY_U: return RETROK_u;
        case godot::Key::KEY_V: return RETROK_v;
        case godot::Key::KEY_W: return RETROK_w;
        case godot::Key::KEY_X: return RETROK_x;
        case godot::Key::KEY_Y: return RETROK_y;
        case godot::Key::KEY_Z: return RETROK_z;
        case godot::Key::KEY_CAPSLOCK: return RETROK_CAPSLOCK;
        case godot::Key::KEY_F1: return RETROK_F1;
        case godot::Key::KEY_F2: return RETROK_F2;
        case godot::Key::KEY_F3: return RETROK_F3;
        case godot::Key::KEY_F4: return RETROK_F4;
        case godot::Key::KEY_F5: return RETROK_F5;
        case godot::Key::KEY_F6: return RETROK_F6;
        case godot::Key::KEY_F7: return RETROK_F7;
        case godot::Key::KEY_F8: return RETROK_F8;
        case godot::Key::KEY_F9: return RETROK_F9;
        case godot::Key::KEY_F10: return RETROK_F10;
        case godot::Key::KEY_F11: return RETROK_F11;
        case godot::Key::KEY_F12: return RETROK_F12;
        case godot::Key::KEY_PRINT: return RETROK_PRINT;
        case godot::Key::KEY_SCROLLLOCK: return RETROK_SCROLLOCK;
        case godot::Key::KEY_PAUSE: return RETROK_PAUSE;
        case godot::Key::KEY_INSERT: return RETROK_INSERT;
        case godot::Key::KEY_HOME: return RETROK_HOME;
        case godot::Key::KEY_PAGEUP: return RETROK_PAGEUP;
        case godot::Key::KEY_DELETE: return RETROK_DELETE;
        case godot::Key::KEY_END: return RETROK_END;
        case godot::Key::KEY_PAGEDOWN: return RETROK_PAGEDOWN;
        case godot::Key::KEY_RIGHT: return RETROK_RIGHT;
        case godot::Key::KEY_LEFT: return RETROK_LEFT;
        case godot::Key::KEY_DOWN: return RETROK_DOWN;
        case godot::Key::KEY_UP: return RETROK_UP;
        case godot::Key::KEY_NUMLOCK: return RETROK_NUMLOCK;
        case godot::Key::KEY_KP_DIVIDE: return RETROK_KP_DIVIDE;
        case godot::Key::KEY_KP_MULTIPLY: return RETROK_KP_MULTIPLY;
        case godot::Key::KEY_KP_SUBTRACT: return RETROK_KP_MINUS;
        case godot::Key::KEY_KP_ADD: return RETROK_KP_PLUS;
        case godot::Key::KEY_KP_ENTER: return RETROK_KP_ENTER;
        case godot::Key::KEY_KP_1: return RETROK_KP1;
        case godot::Key::KEY_KP_2: return RETROK_KP2;
        case godot::Key::KEY_KP_3: return RETROK_KP3;
        case godot::Key::KEY_KP_4: return RETROK_KP4;
        case godot::Key::KEY_KP_5: return RETROK_KP5;
        case godot::Key::KEY_KP_6: return RETROK_KP6;
        case godot::Key::KEY_KP_7: return RETROK_KP7;
        case godot::Key::KEY_KP_8: return RETROK_KP8;
        case godot::Key::KEY_KP_9: return RETROK_KP9;
        case godot::Key::KEY_KP_0: return RETROK_KP0;
        case godot::Key::KEY_KP_PERIOD: return RETROK_KP_PERIOD;
        // case godot::Key::KEY_APPLICATION: return RETROK_COMPOSE;
        // case godot::Key::KEY_POWER: return RETROK_POWER;
        // case godot::Key::KEY_KP_EQUAL: return RETROK_KP_EQUALS;
        case godot::Key::KEY_F13: return RETROK_F13;
        case godot::Key::KEY_F14: return RETROK_F14;
        case godot::Key::KEY_F15: return RETROK_F15;
        case godot::Key::KEY_HELP: return RETROK_HELP;
        case godot::Key::KEY_MENU: return RETROK_MENU;
        // case godot::Key::KEY_UNDO: return RETROK_UNDO;
        case godot::Key::KEY_SYSREQ: return RETROK_SYSREQ;
        case godot::Key::KEY_CTRL: 
            return  location == godot::KeyLocation::KEY_LOCATION_LEFT ?  RETROK_LCTRL : RETROK_RCTRL;
        case godot::Key::KEY_SHIFT: 
            return  location == godot::KeyLocation::KEY_LOCATION_LEFT ?  RETROK_LSHIFT : RETROK_RSHIFT;
         case godot::Key::KEY_ALT: 
            return  location == godot::KeyLocation::KEY_LOCATION_LEFT ?  RETROK_LALT : RETROK_RALT;
        // case godot::Key::KEY_LGUI: return RETROK_LMETA;
        // case godot::Key::KEY_RGUI: return RETROK_RMETA;
        // case godot::Key::KEY_MODE: return RETROK_MODE;
        default: return RETROK_UNKNOWN;
    }
}

RetroInput::Gamepad::Gamepad() {
    deviceIndex = 0;
    // instanceId = 0;
    // controller = nullptr;
    // joystick = nullptr;

    lastDir[0] = 0; lastDir[1] = 0; lastDir[2] = 0;
    lastDir[3] = 0; lastDir[4] = 0; lastDir[5] = 0;

    state[0] = false; state[1] = false; state[2] = false; state[3] = false;
    state[4] = false; state[5] = false; state[6] = false; state[7] = false;
    state[8] = false; state[9] = false; state[10] = false; state[11] = false;
    state[12] = false; state[13] = false; state[14] = false; state[15] = false;

    analogs[0].x = 0; analogs[0].y = 0;
    analogs[1].x = 0; analogs[1].y = 0;
    analogs[2].x = 0; analogs[2].y = 0;

    sensitivity = 0.5f;
    digital = false;
}
