
#include "avisynth.h"
#include "print.h"
#include "median.h"
#include "opt_med.h"
#include <vector>
#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#define _CRT_SECURE_NO_WARNINGS
#endif
//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
Median::Median(PClip _child, std::vector<PClip> _clips, unsigned int _low, unsigned int _high, bool _temporal, bool _processchroma, unsigned int _sync, unsigned int _samples, bool _debug, IScriptEnvironment* env) :
  GenericVideoFilter(_child), clips(_clips), low(_low), high(_high), temporal(_temporal), processchroma(_processchroma), sync(_sync), samples(_samples), debug(_debug)
{
  if (temporal)
    depth = 2 * low + 1; // In this case low == high == radius and we only have one source clip
  else
    depth = (int)clips.size();

  blend = depth - low - high;

  if (blend == 1 && low == high && depth <= MAX_OPT)
    fastprocess = true;
  else
    fastprocess = false;

#ifdef _WIN32
  debugf("depth: %d, blend: %d, low: %d, high: %d, fast: %d, temporal: %d, sync: %d, samples: %d",
    depth, blend, low, high, (int)fastprocess, (int)temporal, (int)sync, (int)samples);
#endif

  switch (depth)
  {
  case 3: fastmedian = opt_med3; break;
  case 5: fastmedian = opt_med5; break;
  case 7: fastmedian = opt_med7; break;
  case 9: fastmedian = opt_med9; break;
  }

  if (temporal)
  {
    info.push_back(clips[0]->GetVideoInfo());
  }
  else // When dealing with more than one source, make sure that they match
  {
    for (unsigned int i = 0; i < depth; i++)
      info.push_back(clips[i]->GetVideoInfo());

    for (unsigned int i = 1; i < depth; i++)
    {
      if (!info[i].IsSameColorspace(info[0]))
        env->ThrowError(ERROR_PREFIX "Format of all clips must match.");
    }

    for (unsigned int i = 1; i < depth; i++)
    {
      if (info[i].width != info[0].width || info[i].height != info[0].height)
        env->ThrowError(ERROR_PREFIX "Dimensions of all clips must match.");
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
Median::~Median()
{
}


//////////////////////////////////////////////////////////////////////////////
// Actual image processing operations
//////////////////////////////////////////////////////////////////////////////
PVideoFrame __stdcall Median::GetFrame(int n, IScriptEnvironment* env)
{
  // Sync statistics for this frame
  double best[MAX_DEPTH] = { 0.0 };
  int match[MAX_DEPTH] = { 0 };

  // Source
  PVideoFrame src[MAX_DEPTH];

  if (temporal)
  {
    unsigned int radius = low; // low == high == radius

    // TODO: Do I need to worry about negative frames or frames after the last? Looks like no
    for (unsigned int i = 0; i < depth; i++)
      src[i] = clips[0]->GetFrame(n - radius + i, env); // Grab an equal number of preceding and following frames
  }
  else if (sync > 0)
  {
    src[0] = clips[0]->GetFrame(n, env);

    for (unsigned int i = 1; i < depth; i++)
    {
      int radius = sync;

      for (int j = -radius; j <= radius; j++)
      {
        double similarity = CompareFrames(PLANAR_Y, src[0], clips[i]->GetFrame(n + j, env), samples);

        if (similarity > best[i])
        {
          best[i] = similarity;
          match[i] = j;
        }
      }

      src[i] = clips[i]->GetFrame(n + match[i], env);
    }
  }
  else
  {
    for (unsigned int i = 0; i < depth; i++)
      src[i] = clips[i]->GetFrame(n, env);
  }

  // Output
  PVideoFrame output = env->NewVideoFrame(vi);

  // Select between planar and interleaved processing
  if (info[0].IsPlanar())
    ProcessPlanarFrame(src, output);
  else
    ProcessInterleavedFrame(src, output);

  // Print debug information on output image
  if (debug)
  {
    line = 0;
    textf(output, "FRAME: %d", n);
    textf(output, "CLIPS: %d", depth);

    if (sync > 0)
    {
      textf(output, "SYNC RADIUS: %d", sync);
      textf(output, "SYNC METRICS:");

      for (unsigned int i = 1; i < depth; i++)
        textf(output, "%-2d %+-3d %-f", i + 1, match[i], best[i]);
    }
  }

  return output;
}


//////////////////////////////////////////////////////////////////////////////
// Compare two frames
// 
// returns 100.0 -> exact match, 0.0 -> completely different
//////////////////////////////////////////////////////////////////////////////
double Median::CompareFrames(int plane, PVideoFrame a, PVideoFrame b, unsigned int points)
{
  const unsigned char* aptr = a->GetReadPtr(plane);
  const unsigned char* bptr = b->GetReadPtr(plane);

  const unsigned int length = a->GetRowSize(plane) * a->GetHeight(plane);

  if (points < 1 || points > length)
    points = length;

  const unsigned int step = length / points;

  unsigned long sum = 0;

  for (unsigned int i = 0; i < length; i = i + step)
    sum = sum + abs((int)aptr[i] - (int)bptr[i]);

  double difference = (100.0 * sum) / (255.0 * points);

  return 100.0 - difference;
}


//////////////////////////////////////////////////////////////////////////////
// Image processing for planar images
//////////////////////////////////////////////////////////////////////////////
void Median::ProcessPlanarFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst)
{
  ProcessPlane(PLANAR_Y, src, dst);
  ProcessPlane(PLANAR_U, src, dst);
  ProcessPlane(PLANAR_V, src, dst);
}


//////////////////////////////////////////////////////////////////////////////
// Processing of a single plane
//////////////////////////////////////////////////////////////////////////////
void Median::ProcessPlane(int plane, PVideoFrame src[MAX_DEPTH], PVideoFrame& dst)
{
  // Source
  const unsigned char* srcp[MAX_DEPTH];

  for (unsigned int i = 0; i < depth; i++)
    srcp[i] = src[i]->GetReadPtr(plane);

  // Destination
  unsigned char* dstp = dst->GetWritePtr(plane);

  // Dimensions
  const int width = src[0]->GetRowSize(plane);
  const int height = src[0]->GetHeight(plane);

  // Process
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      unsigned char values[MAX_DEPTH];

      for (unsigned int i = 0; i < depth; i++)
        values[i] = srcp[i][x];

      if (plane == PLANAR_Y || processchroma == true)
        dstp[x] = ProcessPixel(values);
      else
        dstp[x] = values[0]; // Use values from first clip
    }

    for (unsigned int i = 0; i < depth; i++)
      srcp[i] = srcp[i] + src[i]->GetPitch(plane);

    dstp = dstp + dst->GetPitch(plane);
  }
}


//////////////////////////////////////////////////////////////////////////////
// Image processing for interleaved images
//////////////////////////////////////////////////////////////////////////////
void Median::ProcessInterleavedFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst)
{
  // Source
  const unsigned char* srcp[MAX_DEPTH];

  for (unsigned int i = 0; i < depth; i++)
    srcp[i] = src[i]->GetReadPtr();

  // Destination
  unsigned char* dstp = dst->GetWritePtr();

  // Dimensions
  const int width = info[0].width;
  const int height = info[0].height;

  // Process
  if (info[0].IsYUY2())
  {
    //////////////////////////////////////////////////////////////////////
    // YUYV
    //////////////////////////////////////////////////////////////////////
    unsigned char luma[MAX_DEPTH];
    unsigned char chroma[MAX_DEPTH];

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; x++)
      {
        for (unsigned int i = 0; i < depth; i++)
        {
          luma[i] = srcp[i][x * 2];
          chroma[i] = srcp[i][x * 2 + 1];
        }

        dstp[x * 2] = ProcessPixel(luma);
        dstp[x * 2 + 1] = processchroma ? ProcessPixel(chroma) : chroma[0];
      }

      for (unsigned int i = 0; i < depth; i++)
        srcp[i] = srcp[i] + src[i]->GetPitch();

      dstp = dstp + dst->GetPitch();
    }
  }
  else if (info[0].IsRGB24())
  {
    //////////////////////////////////////////////////////////////////////
    // BGR
    //////////////////////////////////////////////////////////////////////
    unsigned char b[MAX_DEPTH];
    unsigned char g[MAX_DEPTH];
    unsigned char r[MAX_DEPTH];

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; x++)
      {
        for (unsigned int i = 0; i < depth; i++)
        {
          b[i] = srcp[i][x * 3];
          g[i] = srcp[i][x * 3 + 1];
          r[i] = srcp[i][x * 3 + 2];
        }

        dstp[x * 3] = ProcessPixel(b);
        dstp[x * 3 + 1] = ProcessPixel(g);
        dstp[x * 3 + 2] = ProcessPixel(r);
      }

      for (unsigned int i = 0; i < depth; i++)
        srcp[i] = srcp[i] + src[i]->GetPitch();

      dstp = dstp + dst->GetPitch();
    }
  }
  else if (info[0].IsRGB32())
  {
    //////////////////////////////////////////////////////////////////////
    // BGRA
    //////////////////////////////////////////////////////////////////////
    unsigned char b[MAX_DEPTH];
    unsigned char g[MAX_DEPTH];
    unsigned char r[MAX_DEPTH];
    unsigned char a[MAX_DEPTH];

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; x++)
      {
        for (unsigned int i = 0; i < depth; i++)
        {
          b[i] = srcp[i][x * 4];
          g[i] = srcp[i][x * 4 + 1];
          r[i] = srcp[i][x * 4 + 2];
          a[i] = srcp[i][x * 4 + 3];
        }

        dstp[x * 4] = ProcessPixel(b);
        dstp[x * 4 + 1] = ProcessPixel(g);
        dstp[x * 4 + 2] = ProcessPixel(r);
        dstp[x * 4 + 3] = processchroma ? ProcessPixel(a) : a[0];
      }

      for (unsigned int i = 0; i < depth; i++)
        srcp[i] = srcp[i] + src[i]->GetPitch();

      dstp = dstp + dst->GetPitch();
    }
  }
  else if (info[0].pixel_type == VideoInfo::CS_BGR64)
  {
    //////////////////////////////////////////////////////////////////////
    // BGRA
    //////////////////////////////////////////////////////////////////////
    uint16_t b_16bit[MAX_DEPTH];
    uint16_t g_16bit[MAX_DEPTH];
    uint16_t r_16bit[MAX_DEPTH];
    uint16_t a_16bit[MAX_DEPTH];

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; x++)
      {
        for (unsigned int i = 0; i < depth; i++)
        {
          b_16bit[i] = srcp[i][x * 8 + 0] | (srcp[i][x * 8 + 1] << 8);
          g_16bit[i] = srcp[i][x * 8 + 2] | (srcp[i][x * 8 + 3] << 8);
          r_16bit[i] = srcp[i][x * 8 + 4] | (srcp[i][x * 8 + 5] << 8);
          a_16bit[i] = srcp[i][x * 8 + 6] | (srcp[i][x * 8 + 7] << 8);
        }

        uint16_t median_b = ProcessPixel_16bit(b_16bit);
        uint16_t median_g = ProcessPixel_16bit(g_16bit);
        uint16_t median_r = ProcessPixel_16bit(r_16bit);
        uint16_t median_a = ProcessPixel_16bit(a_16bit);

        dstp[x * 8] = static_cast<BYTE>(median_b);
        dstp[x * 8 + 1] = static_cast<BYTE>(median_b >> 8);
        dstp[x * 8 + 2] = static_cast<BYTE>(median_g);
        dstp[x * 8 + 3] = static_cast<BYTE>(median_g >> 8);
        dstp[x * 8 + 4] = static_cast<BYTE>(median_r);
        dstp[x * 8 + 5] = static_cast<BYTE>(median_r >> 8);
        dstp[x * 8 + 6] = static_cast<BYTE>(processchroma ? median_a : a_16bit[0]);
        dstp[x * 8 + 7] = static_cast<BYTE>(processchroma ? median_a >> 8 : a_16bit[0] >> 8);
      }

      for (unsigned int i = 0; i < depth; i++)
        srcp[i] = srcp[i] + src[i]->GetPitch();

      dstp = dstp + dst->GetPitch();
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
// Processing of a stack of pixel values
//////////////////////////////////////////////////////////////////////////////
inline uint16_t Median::ProcessPixel_16bit(uint16_t* values) const
{
  uint16_t output;

  unsigned int sum = 0;

  if (blend != depth) // If all clips are to be blended, there is no need to sort them
    std::sort(values, values + depth);

  for (unsigned int i = low; i < low + blend; i++)
    sum = sum + values[i];

  output = sum / blend;

  return output;
}

inline unsigned char Median::ProcessPixel(unsigned char* values) const
{
  unsigned char output;

  if (fastprocess) // Can use a fast method
  {
    output = fastmedian(values);
  }
  else // Full processing
  {
    unsigned int sum = 0;

    if (blend != depth) // If all clips are to be blended, there is no need to sort them
      std::sort(values, values + depth);

    for (unsigned int i = low; i < low + blend; i++)
      sum = sum + values[i];

    output = sum / blend;
  }

  return output;
}


#ifdef _WIN32
//////////////////////////////////////////////////////////////////////////////
// Print things to be viewed in DebugView
//////////////////////////////////////////////////////////////////////////////
void Median::debugf(const char* fmt, ...)
{
  if (debug)
  {
    char buffer[1024] = { "median: " };
    char* ptr = buffer + strlen(buffer);

    va_list args;
    va_start(args, fmt);
    vsnprintf(ptr, sizeof(buffer), fmt, args);
    va_end(args);

    OutputDebugStringA(buffer);
  }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// Print things on top of image
//////////////////////////////////////////////////////////////////////////////
void Median::textf(PVideoFrame& dst, const char* fmt, ...)
{
  char string[1024] = { 0 };

  int n = info[0].width / FONT_WIDTH;

  va_list args;
  va_start(args, fmt);
  vsnprintf(string, sizeof(string), fmt, args);
  va_end(args);

  if (info[0].IsYUY2()) print_yuyv(dst, line, string);
  else if (info[0].IsRGB24()) print_rgb(dst, line, string, false);
  else if (info[0].IsRGB32()) print_rgb(dst, line, string, true);
  else if (info[0].IsPlanar()) print_planar(dst, line, string);

  line++;
}
