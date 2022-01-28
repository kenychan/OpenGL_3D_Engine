#pragma once

// Standard Headers
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <sstream>

// Vendor Headers
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "config.hpp"

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include <cstdlib>


GLFWwindow*
initOpenGL(int width, int height, const char* title);
