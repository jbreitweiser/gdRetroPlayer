#include "Perf.hpp"

#include <time.h>

bool Perf::init(Logger *logger) {
   _logger = logger;
    return true;
}

void Perf::destroy() {}

uint64_t Perf::getTimeUs() {
    struct timespec ts = {0};

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return static_cast<int64_t>(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

uint64_t Perf::getTimeNs() {
    struct timespec ts = {0};

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return static_cast<int64_t>(ts.tv_sec * 1000000000 + ts.tv_nsec);
}

// RETRO_ENVIRONMENT_GET_PERF_INTERFACE
retro_time_t Perf::getTimeUsec() {
    return static_cast<retro_time_t>(getTimeUs());
}

// RETRO_ENVIRONMENT_GET_PERF_INTERFACE
uint64_t Perf::getCpuFeatures() {
    godot::UtilityFunctions::print("Perf::getCpuFeatures :: Called");
    if ( _cpuId.init(_logger)) {
        return _cpuId.getFeatures();
    }
    else
    {
        _logger->error("Perf::getCpuFeatures :: Failed to init CpuId");
    }

    return 0;
}

retro_perf_tick_t Perf::getCounter() {
    return static_cast<retro_perf_tick_t>(getTimeNs());
}

void Perf::register_(retro_perf_counter* counter) {
    (void)counter;
}

void Perf::start(retro_perf_counter* counter) {
    (void)counter;
}

void Perf::stop(retro_perf_counter* counter) {
    (void)counter;
}

void Perf::log() {}
