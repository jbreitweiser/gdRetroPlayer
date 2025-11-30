#pragma once

#include <lrcpp/libretro.h>
#include <stdint.h>
#include <memory>

#include "Logger.hpp"

class CpuIdImpl; 

class CpuId {
public:
    CpuId();
    ~CpuId();
    bool init(Logger* logger);
    void destroy();

    uint64_t getFeatures();

private:
	std::unique_ptr<CpuIdImpl> pimpl;
};

