%module glfw3
%insert("include")
%{
#include <GLFW/glfw3.h>
%}

#define SWIG_FORTH_GFORTH_LIBRARY "glfw"
#define SWIG_FORTH_GFORTH_INCLUDE "GLFW/glfw3.h"

%include <GLFW/glfw3.h>