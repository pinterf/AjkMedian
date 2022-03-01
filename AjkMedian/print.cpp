#include "print.h"
#include "font.h"
#include <cstring>
#include <stdlib.h>
#include <algorithm>

static int string_to_bitmap(const char* string, unsigned char*& text_buffer)
{
  unsigned int text_width = (int)strlen(string) * FONT_WIDTH;

  text_buffer = (unsigned char*)malloc(text_width * FONT_HEIGHT);

  unsigned char* pixel = text_buffer;

  for (int y = 0; y < FONT_HEIGHT; y++)
  {
    for (size_t n = 0; n < strlen(string); n++)
    {
      for (int x = 0; x < FONT_WIDTH; x++)
      {
        unsigned char b = font8x8[string[n]][y] & (1U << x);
        *pixel++ = b ? 255 : 0;
      }
    }
  }

  return text_width;
}


//////////////////////////////////////////////////////////////////////////////
// Interleaved YUY2
//////////////////////////////////////////////////////////////////////////////
void print_yuyv(PVideoFrame& dst, unsigned int line, const char* string)
{
  unsigned char* text_buffer;

  int text_width = string_to_bitmap(string, text_buffer);
  int image_width = dst->GetPitch() / 2;

  unsigned char* row = dst->GetWritePtr() + (line * FONT_HEIGHT * FONT_SCALE * dst->GetPitch());
  unsigned char* end = dst->GetWritePtr() + (dst->GetHeight() * dst->GetPitch()) - 1;

  for (int y = 0; y < FONT_HEIGHT; y++)
  {
    unsigned char* text = text_buffer + y * text_width;

    for (int n = 0; n < FONT_SCALE; n++)
    {
      unsigned char* pixel = row;

      for (int x = 0; x < std::min(text_width, image_width); x++)
      {
        *pixel++ = text[x]; // Y
        *pixel++ = 128;     // U/V = neutral
      }

      row = row + dst->GetPitch();

      if (row > end)
        goto done;
    }
  }

done:

  free(text_buffer);
}


//////////////////////////////////////////////////////////////////////////////
// Interleaved RGB(A) - image is flipped vertically compared to YUY2
//////////////////////////////////////////////////////////////////////////////
void print_rgb(PVideoFrame& dst, unsigned int line, const char* string, bool alpha)
{
  unsigned char* text_buffer;

  int text_width = string_to_bitmap(string, text_buffer);
  int image_width = dst->GetPitch() / (alpha ? 4 : 3);

  unsigned char* end = dst->GetWritePtr();
  unsigned char* row = dst->GetWritePtr() + ((dst->GetHeight() - FONT_SCALE * FONT_HEIGHT * line - 1) * dst->GetPitch());

  for (int y = 0; y < FONT_HEIGHT; y++)
  {
    unsigned char* text = text_buffer + y * text_width;

    for (int n = 0; n < FONT_SCALE; n++)
    {
      if (row < end)
        goto done;

      unsigned char* pixel = row;

      for (int x = 0; x < std::min(text_width, image_width); x++)
      {
        *pixel++ = text[x]; // R
        *pixel++ = text[x]; // G
        *pixel++ = text[x]; // B

        if (alpha)
          *pixel++ = 255; // A
      }

      row = row - dst->GetPitch();

    }
  }

done:

  free(text_buffer);
}


//////////////////////////////////////////////////////////////////////////////
// Planar colourspaces
//////////////////////////////////////////////////////////////////////////////
void print_planar(PVideoFrame& dst, unsigned int line, const char* string)
{
  unsigned char* text_buffer;

  int text_width = string_to_bitmap(string, text_buffer);
  int image_width = dst->GetPitch();

  unsigned char* row = dst->GetWritePtr(PLANAR_Y) + (line * FONT_HEIGHT * FONT_SCALE * dst->GetPitch(PLANAR_Y));
  unsigned char* end = dst->GetWritePtr(PLANAR_Y) + (dst->GetHeight(PLANAR_Y) * dst->GetPitch(PLANAR_Y)) - 1;

  for (int y = 0; y < FONT_HEIGHT; y++)
  {
    unsigned char* text = text_buffer + y * text_width;

    for (int n = 0; n < FONT_SCALE; n++)
    {
      unsigned char* pixel = row;

      for (int x = 0; x < std::min(text_width, image_width); x++)
        *pixel++ = text[x]; // Y

      row = row + dst->GetPitch();

      if (row > end)
        goto done;
    }
  }

done:

  free(text_buffer);
}
