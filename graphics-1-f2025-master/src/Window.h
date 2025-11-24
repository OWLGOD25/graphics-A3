#pragma once

void CreateWindow(int width, int height, const char* title);
void DestroyWindow();

bool WindowShouldClose();
void Loop();
