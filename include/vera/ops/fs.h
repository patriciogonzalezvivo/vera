#pragma once

#include "string.h"

namespace vera {

// =============================================================================
// FILE OPERATIONS
// =============================================================================

/// Check if a URL or file exists
/// @param _filename URL or file path to check
/// @return True if the file/URL exists and is accessible
bool urlExists(const std::string& _filename);

/// Check if a filename has a specific extension
/// @param _filename File path to check
/// @param _ext Extension to look for (with or without leading dot)
/// @return True if file has the specified extension
bool haveExt(const std::string& _filename, const std::string& _ext);

/// Get the file extension from a filename
/// @param _filename File path
/// @return Extension including the dot (e.g., ".txt") or empty string
std::string getExt(const std::string& _filename);

/// Extract filename from a full file path
/// @param _filepath Full path to file
/// @return Just the filename with extension, without directory path
std::string getFilename(const std::string& _filepath);

/// Check if path refers to a directory
/// @param _path Path to check
/// @return True if path is a directory
bool isFolder(const std::string& _path);

// =============================================================================
// PATH OPERATIONS
// =============================================================================

/// Get the base directory from a file path
/// @param filepath Full file path
/// @return Directory portion of the path
std::string getBaseDir(const std::string& filepath);

/// Get the absolute path from a relative or absolute path
/// @param _filename File path (relative or absolute)
/// @return Absolute path
std::string getAbsPath(const std::string& _filename);

/// Resolve a URL/path using current working directory and include folders
/// @param _filename File to resolve
/// @param _pwd Current working directory
/// @param _include_folders List of directories to search
/// @return Resolved absolute path or original if not found
std::string urlResolve(const std::string& _filename, const std::string& _pwd, const StringList& _include_folders);

/// Find files matching a glob pattern
/// @param _pattern Glob pattern (supports * and ? wildcards)
/// @return Vector of matching file paths
std::vector<std::string> glob(const std::string& _pattern);

// =============================================================================
// GLSL SOURCE CODE FILES
// =============================================================================

/// Load GLSL shader source from file
/// @param _path Path to GLSL file
/// @return Complete GLSL source code with includes resolved
std::string loadGlslFrom(const std::string& _path);

/// Load GLSL shader source with dependencies tracking
/// @param _path Path to GLSL file
/// @param _include_folders Directories to search for includes
/// @param _dependencies Output list of all included files
/// @return Complete GLSL source code with includes resolved
std::string loadGlslFrom(const std::string& _path, const StringList& _include_folders, StringList *_dependencies);

/// Load GLSL shader source into existing string  
/// @param _path Path to GLSL file
/// @param _into Output string to append to
/// @return True if successfully loaded
bool loadGlslFrom(const std::string& _path, std::string *_into);

/// Load GLSL shader source with full options
/// @param _filename Path to GLSL file
/// @param _into Output string to append to
/// @param _include_folders Directories to search for includes
/// @param _dependencies Output list of all included files
/// @return True if successfully loaded
bool loadGlslFrom(const std::string& _filename, std::string *_into, const StringList& _include_folders, StringList *_dependencies);

/// Resolve #include directives in GLSL source
/// @param _src GLSL source code
/// @return Source with includes resolved
std::string resolveGlsl(const std::string& _src);

/// Resolve #include directives with dependencies tracking
/// @param _src GLSL source code
/// @param _include_folders Directories to search for includes
/// @param _dependencies Output list of all included files
/// @return Source with includes resolved
std::string resolveGlsl(const std::string& _src, const StringList& _include_folders, StringList *_dependencies);

/// Resolve #include directives into existing string
/// @param _src GLSL source code
/// @param _into Output string to append to
/// @param _include_folders Directories to search for includes
/// @param _dependencies Output list of all included files
/// @return True if successfully resolved
bool resolveGlsl(const std::string& _src, std::string *_into, const StringList& _include_folders, StringList *_dependencies);

/// Resolve #include directives with current directory context
/// @param _src GLSL source code
/// @param _pwd Current working directory for relative paths
/// @param _into Output string to append to
/// @param _include_folders Directories to search for includes
/// @param _dependencies Output list of all included files
/// @return True if successfully resolved
bool resolveGlsl(const std::string& _src, const std::string& _pwd, std::string *_into, const StringList& _include_folders, StringList *_dependencies);

// =============================================================================
// BINARY DATA ENCODING
// =============================================================================

/// Encode binary data to Base64 string
/// @param _src Pointer to binary data
/// @param _size Size of data in bytes
/// @return Base64-encoded string
std::string encodeBase64(const unsigned char* _src, size_t _size);

/// Decode Base64 string to binary data
/// @param _src Base64-encoded string
/// @param _to Output buffer for decoded data (must be pre-allocated)
/// @return Number of bytes decoded
size_t decodeBase64(const std::string& _src, unsigned char *_to);

}