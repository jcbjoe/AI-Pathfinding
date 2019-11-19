// Strip commonly unused stuff that is inderectly included from windows.h
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// C++ standard library includes
#include <stdio.h>
#include <stdint.h>

// Windows includes
#include <wincodec.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <directxmath.h>

using namespace DirectX;

#include "../src/app.h"
#include "../src/debug.h"
#include "../src/sprite_batch.h"
#include "../src/util.h"