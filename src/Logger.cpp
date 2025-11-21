#include "Logger.h"

void Logger::setLevel(retro_log_level level) {
    _priority = level;
}

void Logger::vprintf(retro_log_level level, char const* format, va_list args) {
    if (_priority >= level)
    {
        // Format the string with the va_list arguments
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        
        switch (level) {
            case RETRO_LOG_DEBUG:
                godot::UtilityFunctions::print(buffer);
                break;
            case RETRO_LOG_INFO:
                godot::UtilityFunctions::print(buffer);
                break;
            case RETRO_LOG_WARN:
                godot::UtilityFunctions::push_warning(buffer); 
                break;
            case RETRO_LOG_ERROR:
                godot::UtilityFunctions::push_error(buffer);
                break;
            default:
                break;

        }
    }
}



