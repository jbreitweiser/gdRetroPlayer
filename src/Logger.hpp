#pragma once

#include <lrcpp/Components.h>

#include <godot_cpp\variant\utility_functions.hpp>
#include <cstdarg>
#include <cstdio>

/*  Log Levels,  Using the libretro levels for compatibility

    RETRO_LOG_DEBUG
    RETRO_LOG_INFO
    RETRO_LOG_WARN
    RETRO_LOG_ERROR

*/
class Logger : public lrcpp::Logger {
public:
    void setLevel(retro_log_level level);

    // lrcpp::Logger
    virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

protected:
    retro_log_level _priority = RETRO_LOG_INFO;
};
