#pragma once

#include <d3d9.h>
#include <windows.h>

// Include guards to prevent multiple inclusion
#ifndef LEFT_SIDE_LOADER_IMAGE_HPP
#define LEFT_SIDE_LOADER_IMAGE_HPP

// Forward declarations
extern LPDIRECT3DTEXTURE9 g_LeftImageTexture;

// Function declarations
void LoadLeftSideImage();
void CleanupLeftSideImage();

// Image data declaration (the actual bytes will be in a separate .cpp file)
extern const unsigned char left_side_loader_image_data[];
extern const unsigned int left_side_loader_image_size;

#endif // LEFT_SIDE_LOADER_IMAGE_HPP 