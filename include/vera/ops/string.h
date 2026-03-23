#pragma once

#include <vector>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "glm/glm.hpp"

namespace vera {

// =============================================================================
// STRING INSPECTION AND PARSING
// =============================================================================

/// Extract version information from a program string
/// @param program The program string to parse
/// @param _version Output parameter for the parsed version number
/// @return The program string with version removed
std::string getVersion(const std::string& program, size_t& _version);

/// Get a specific line from source code
/// @param _source The source code string
/// @param _lineNumber The 1-based line number to extract
/// @return The requested line or empty string if not found
std::string getLineNumber(const std::string& _source, unsigned _lineNumber);

// =============================================================================
// STRING TRANSFORMATION
// =============================================================================

/// Convert string to uppercase
/// @param _string Input string
/// @return New string with all letters converted to uppercase
std::string toUpper(const std::string& _string);

/// Convert string to lowercase
/// @param _string Input string
/// @return New string with all letters converted to lowercase
std::string toLower(const std::string& _string);

/// Replace spaces with underscores
/// @param _string Input string
/// @return New string with spaces replaced by underscores
std::string toUnderscore(const std::string& _string);

/// Remove non-standard characters from a string
/// @param _string Input string
/// @return Purified string with only standard ASCII characters
std::string purifyString(const std::string& _string);

// =============================================================================
// STRING MATCHING AND SEARCHING
// =============================================================================

/// Check if string contains wildcard characters (* or ?)
/// @param _str String to check
/// @return True if string contains * or ? wildcards
bool haveWildcard(const std::string& _str);

/// Split a string into tokens using a delimiter
/// @param _string String to split
/// @param _sep Delimiter character
/// @param _tolerate_empty If true, include empty tokens in result
/// @return Vector of string tokens
std::vector<std::string> split(const std::string& _string, char _sep, bool _tolerate_empty = false);

/// Replace all occurrences of a substring with another string
/// @param _string Input string to process
/// @param _from Substring to find
/// @param _to Replacement string
/// @return New string with all replacements made
std::string replaceAll(std::string _string, const std::string& _from, const std::string& _to);

/// Check if string A begins with string B
/// @param _stringA String to check
/// @param _stringB Prefix to look for
/// @return True if _stringA starts with _stringB
bool beginsWith(const std::string& _stringA, const std::string& _stringB);

// =============================================================================
// TYPE CHECKING
// =============================================================================

/// Check if string represents an integer
/// @param _string String to check
/// @return True if string is a valid integer
bool isInt(const std::string& _string);

/// Check if all characters are digits
/// @param _string String to check
/// @return True if all characters are 0-9
bool isDigit(const std::string& _string);

/// Check if string represents any numeric value
/// @param _string String to check
/// @return True if string is a valid number (int or float)
bool isNumber(const std::string& _string);

/// Check if string represents a floating-point number
/// @param _string String to check
/// @return True if string is a valid float
/// Check if string represents a floating-point number
/// @param _string String to check
/// @return True if string is a valid float
bool isFloat(const std::string& _string);

// =============================================================================
// TYPE CONVERSIONS
// =============================================================================

/// Convert string to boolean
/// @param _string String to convert (accepts "true", "false", "1", "0", etc.)
/// @return Boolean value
bool toBool(const std::string& _string);

/// Convert string to character
/// @param _string String to convert
/// @return First character of string or '\0' if empty
char toChar(const std::string& _string);

/// Convert string to integer
/// @param _string String to convert
/// @return Integer value or 0 if conversion fails
int toInt(const std::string& _string);

/// Convert string to float
/// @param _string String to convert
/// @return Float value or 0.0f if conversion fails
float toFloat(const std::string& _string);

/// Convert string to double
/// @param _string String to convert
/// @return Double value or 0.0 if conversion fails
double toDouble(const std::string& _string);

/// Convert boolean to string
/// @param _bool Boolean value
/// @return "true" or "false"
std::string toString(bool _bool);

/// Convert any value to string using stream operator
/// @tparam T Type of value to convert
/// @param _value Value to convert
/// @return String representation
template <class T>
std::string toString(const T& _value) {
    std::ostringstream out;
    out << std::fixed << _value;
    return out.str();
}

/// Convert value to string with specified precision  
/// @tparam T Type of value to convert
/// @param _value Value to convert
/// @param _precision Number of decimal places (like sprintf "%.4f")
/// @return Formatted string
template <class T>
inline std::string toString(const T& _value, int _precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(_precision) << _value;
    return out.str();
}

/// Convert value to string with specified width and fill character
/// @tparam T Type of value to convert
/// @param _value Value to convert  
/// @param _width Minimum field width (like sprintf "%4d")
/// @param _fill Fill character for padding
/// @return Formatted string
template <class T>
inline std::string toString(const T& _value, int _width, char _fill) {
    std::ostringstream out;
    out << std::fixed << std::setfill(_fill) << std::setw(_width) << _value;
    return out.str();
}

/// Convert value to string with precision, width and fill
/// @tparam T Type of value to convert
/// @param _value Value to convert
/// @param _precision Number of decimal places
/// @param _width Minimum field width (like sprintf "%04.2f")
/// @param _fill Fill character for padding
/// @return Formatted string
template <class T>
inline std::string toString(const T& _value, int _precision, int _width, char _fill) {
    std::ostringstream out;
    out << std::fixed << std::setfill(_fill) << std::setw(_width) << std::setprecision(_precision) << _value;
    return out.str();
}

/// Convert vec2 to string
/// @param _vec Vector to convert
/// @param _sep Separator character between components
/// @param _precision Number of decimal places
/// @return String representation (e.g., "1.0,2.0")
std::string toString(const glm::vec2& _vec, char _sep = ',', int _precision = 3);

/// Convert vec3 to string
/// @param _vec Vector to convert
/// @param _sep Separator character between components
/// @param _precision Number of decimal places
/// @return String representation (e.g., "1.0,2.0,3.0")
std::string toString(const glm::vec3& _vec, char _sep = ',', int _precision = 3);

/// Convert vec4 to string
/// @param _vec Vector to convert
/// @param _sep Separator character between components
/// @param _precision Number of decimal places
/// @return String representation (e.g., "1.0,2.0,3.0,4.0")
std::string toString(const glm::vec4& _vec, char _sep = ',', int _precision = 3);

/// Convert mat4 to string
/// @param _mat Matrix to convert
/// @param _sep Separator character between elements
/// @param _precision Number of decimal places
/// @return String representation of 4x4 matrix
std::string toString(const glm::mat4& _mat, char _sep = ' ', int _precision = 3);

// =============================================================================
// STREAM OPERATORS
// =============================================================================

/// Output stream operator for vec3
inline std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);

/// Input stream operator for vec3
inline std::istream& operator>>(std::istream& is, glm::vec3& vec);

// =============================================================================
// STRING LIST OPERATIONS
// =============================================================================

typedef std::vector<std::string> StringList;

/// Merge two string lists, removing duplicates
/// @param _A First string list
/// @param _B Second string list
/// @return Merged list with unique strings
StringList mergeLists(const StringList &_A,const StringList &_B);

/// Add string to list if not already present
/// @param _str String to add
/// @param _list List to add to
void addListElement(const std::string &_str, StringList &_list);

/// Remove string from list if present
/// @param _str String to remove
/// @param _list List to remove from
void delListElement(const std::string &_str, StringList &_list);

/// Extract uniform name from GLSL variable declaration
/// @param _str GLSL uniform declaration string
/// @return Extracted uniform name
std::string getUniformName(const std::string& _str);

}