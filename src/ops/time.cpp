#include "vera/ops/time.h"

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #include <chrono>
    #include <thread>
    #include <ctime>
#else
    #include <unistd.h>
    #include <time.h>
#endif 

// Standard C++ libraries
namespace vera {

void sleep_ms(uint64_t value) {
#if defined(_WIN32)
    std::this_thread::sleep_for(std::chrono::microseconds(value));
#else
    usleep(value);
#endif 
}

std::string getDateTimeString(const std::string& format) {
    time_t now = time(nullptr);
    struct tm tstruct;
    char buf[80];
    #ifdef _WIN32
    localtime_s(&tstruct, &now);
    #else
    localtime_r(&now, &tstruct);
    #endif
    strftime(buf, sizeof(buf), format.c_str(), &tstruct);
    return std::string(buf);
}

}