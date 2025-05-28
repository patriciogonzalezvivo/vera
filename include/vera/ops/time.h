#pragma once

#include <cinttypes>
#include <string>

namespace vera {

void sleep_ms(uint64_t value);

std::string getDateTimeString(const std::string& format = "%Y-%m-%d %H:%M:%S");

}