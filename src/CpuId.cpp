#include "CpuId.hpp"

#include "../external/libcpuid/libcpuid.h"
#include "DynLib.hpp"

struct LibCpuId final {
	bool (*cpuid_present)();  // cpuid_present()
	int (*cpuid_get_raw_data)(struct cpu_raw_data_t*); // cpuid_get_raw_data()
	const char* (*cpuid_error)(); 
	int (*cpu_identify)(struct cpu_raw_data_t*, struct cpu_id_t*);  // int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data);
};

class CpuIdImpl {
public:
    bool init(Logger* logger);
    void destroy();

    uint64_t getFeatures();

private:
	Logger *_logger;
    DynLib _cpulib;
	LibCpuId _libCpuId;
};

CpuId::CpuId() : pimpl(std::make_unique<CpuIdImpl>()) {}

CpuId::~CpuId() {
    destroy();
};

bool CpuId::init(Logger *logger) {
    return pimpl->init(logger);
}

void CpuId::destroy() {
    pimpl->destroy();
}

uint64_t CpuId::getFeatures() {
    return pimpl->getFeatures();
}

bool CpuIdImpl::init(Logger *logger) {
    _logger = logger;

    char const *path = "C:\\msys64\\home\\jbrei\\gdRetroPlayer2\\demo\\addons\\libRetroPlayer\\bin\\libcpuid-17.dll";
    if (!_cpulib.load(path)) {
        _logger->error("CpuId::init :: Failed to load library");
        return false;
    }

    _libCpuId.cpuid_present = reinterpret_cast<decltype(_libCpuId.cpuid_present)>(_cpulib.getSymbol("cpuid_present"));
    _libCpuId.cpuid_get_raw_data = reinterpret_cast<decltype(_libCpuId.cpuid_get_raw_data)>(_cpulib.getSymbol("cpuid_get_raw_data"));
    _libCpuId.cpuid_error = reinterpret_cast<decltype(_libCpuId.cpuid_error)>(_cpulib.getSymbol("cpuid_error"));
    _libCpuId.cpu_identify = reinterpret_cast<decltype(_libCpuId.cpu_identify)>(_cpulib.getSymbol("cpu_identify"));
    return true;
 
}

void CpuIdImpl::destroy() {
    _cpulib.unload();
    _libCpuId.cpuid_present = nullptr; 
	_libCpuId.cpuid_get_raw_data = nullptr;
	_libCpuId.cpuid_error = nullptr;
	_libCpuId.cpu_identify = nullptr;
}

uint64_t CpuIdImpl::getFeatures() {
    
    if(_libCpuId.cpuid_present == nullptr){
        _logger->error("Sorry, no CPU detected!\n");
        return 0;
    }
    
    if (!_libCpuId.cpuid_present()) {                                                // check for CPUID presence
		_logger->error("Sorry, your CPU doesn't support CPUID!\n");
		return 0;
	}

    struct cpu_raw_data_t raw;                                           // contains only raw data
	struct cpu_id_t data;                                                // contains recognized CPU features data

	if (_libCpuId.cpuid_get_raw_data(&raw) < 0) {                        // obtain the raw CPUID data
		_logger->error("Sorry, cannot get the CPUID raw data.\n");
		_logger->error("Error: %s", _libCpuId.cpuid_error());                      // cpuid_error() gives the last error description
		return 0;
	}

	if (_libCpuId.cpu_identify(&raw, &data) < 0) {                       // identify the CPU, using the given raw data.
		_logger->error("Sorrry, CPU identification failed.\n");
		_logger->error("Error: %s", _libCpuId.cpuid_error());
		return 0;
	}

    _logger->debug("Found: %s CPU\n", data.vendor_str);                    // print out the vendor string (e.g. `GenuineIntel')
	_logger->debug("Processor model is `%s'\n", data.cpu_codename);        // print out the CPU code name (e.g. `Pentium 4 (Northwood)')
	_logger->debug("The full brand string is `%s'\n", data.brand_str);
    _logger->debug("Supported multimedia instruction sets:\n");
	_logger->debug("  MMX         : %s\n", data.flags[CPU_FEATURE_MMX] ? "present" : "absent");
	_logger->debug("  MMX-extended: %s\n", data.flags[CPU_FEATURE_MMXEXT] ? "present" : "absent");
	_logger->debug("  SSE         : %s\n", data.flags[CPU_FEATURE_SSE] ? "present" : "absent");
	_logger->debug("  SSE2        : %s\n", data.flags[CPU_FEATURE_SSE2] ? "present" : "absent");
	_logger->debug("  3DNow!      : %s\n", data.flags[CPU_FEATURE_3DNOW] ? "present" : "absent");
    
    uint64_t features = 0;
    features |= data.flags[CPU_FEATURE_AVX] ? RETRO_SIMD_AVX : 0;
    features |= data.flags[CPU_FEATURE_AVX2] ? RETRO_SIMD_AVX2 : 0;
    features |= data.flags[CPU_FEATURE_MMX]  ? RETRO_SIMD_MMX : 0;
    features |= data.flags[CPU_FEATURE_SSE] ? RETRO_SIMD_SSE : 0;
    features |= data.flags[CPU_FEATURE_SSE2] ? RETRO_SIMD_SSE2 : 0;
    features |= data.flags[CPU_FEATURE_SSSE3] ? RETRO_SIMD_SSSE3 : 0;
    features |= data.flags[CPU_FEATURE_SSE4_2] ? RETRO_SIMD_SSE42 : 0;
    features |= data.flags[CPU_FEATURE_VMX] ?  RETRO_SIMD_VMX : 0;
    features |= data.flags[CPU_FEATURE_SSE4_1] ?  RETRO_SIMD_SSE4 : 0;
    features |= data.flags[CPU_FEATURE_MMXEXT] ?  RETRO_SIMD_MMXEXT : 0;
    features |= data.flags[CPU_FEATURE_FPU] ?  RETRO_SIMD_VFPU : 0;
    features |= data.flags[CPU_FEATURE_AES] ?  RETRO_SIMD_AES : 0;
    features |= data.flags[CPU_FEATURE_POPCNT] ?  RETRO_SIMD_POPCNT : 0;
    features |= data.flags[CPU_FEATURE_MOVBE] ?  RETRO_SIMD_MOVBE : 0;
    features |= data.flags[CPU_FEATURE_CMOV] ?  RETRO_SIMD_CMOV : 0;
    return features;
    // These features did not have a libcpu equivalent:
    // #define RETRO_SIMD_VMX128 
    // #define RETRO_SIMD_NEON  
    // #define RETRO_SIMD_SSE3  
    // #define RETRO_SIMD_SSE4 
    // #define RETRO_SIMD_PS 
    // #define RETRO_SIMD_VFPV3 
    // #define RETRO_SIMD_VFPV4 
    // #define RETRO_SIMD_ASIMD 
}

