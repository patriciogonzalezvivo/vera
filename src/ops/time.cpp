#include "vera/ops/time.h"

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #include <chrono>
    #include <thread>
#else
    #include <unistd.h>
#endif 
#include <time.h>

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
    localtime_r(&now, &tstruct);
    strftime(buf, sizeof(buf), format.c_str(), &tstruct);
    return std::string(buf);
}

}