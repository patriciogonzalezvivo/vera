#pragma once

#include <cinttypes>
#include <string>

namespace vera {

/// Sleep for a specified number of milliseconds
/// @param value Duration to sleep in milliseconds
void sleep_ms(uint64_t value);

/// Get the current date and time as a formatted string
/// @param format strftime-compatible format string (default: "%Y-%m-%d %H:%M:%S")
/// @return Formatted date/time string
/// @see strftime(3) for format specifiers
std::string getDateTimeString(const std::string& format = "%Y-%m-%d %H:%M:%S");

}