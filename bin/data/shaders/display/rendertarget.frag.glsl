#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

#define DEFERRED_SAMPLING 0
#define MULTISAMPLING 0

#include "./renderTarget.h"
