#ifndef PRINT_H
#define PRINT_H

#define FONT_WIDTH  8
#define FONT_HEIGHT 8
#define FONT_SCALE  2

#include "avisynth.h"

void print_yuyv(PVideoFrame& dst, unsigned int line, const char* string);
void print_rgb(PVideoFrame& dst, unsigned int line, const char* string, bool alpha);
void print_planar(PVideoFrame& dst, unsigned int line, const char* string);
void print_rgb64(PVideoFrame& dst, unsigned int line, const char* string);

#endif // PRINT_H
