#ifndef PRINT_H
#define PRINT_H

#include "stdafx.h"

#define FONT_WIDTH  8
#define FONT_HEIGHT 8
#define FONT_SCALE  2

void print_yuyv(PVideoFrame& dst, unsigned int line, const char* string);
void print_rgb(PVideoFrame& dst, unsigned int line, const char* string, bool alpha);
void print_planar(PVideoFrame& dst, unsigned int line, const char* string);

#endif // PRINT_H
