#pragma once
#include <d3d9.h>

// Placeholder for right side image bytes - replace with your actual image data
extern unsigned char right_side_image_bytes[];

// Function declarations
void LoadRightSideImage();
void CleanupRightSideImage();

// Global texture variable
extern LPDIRECT3DTEXTURE9 g_RightImageTexture;
