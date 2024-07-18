#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <cstdio>

#include <unordered_map>
#include <vector>
#include <list>
#include <string>

#define XM_ALIGNED_STRUCT(V) __declspec(align(V))