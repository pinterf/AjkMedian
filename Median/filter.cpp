//////////////////////////////////////////////////////////////////////////////
// Median filter for AviSynth
//
// This filter will take in a number of clips and calculate a pixel-by-pixel
// median out of them. This is useful for reducing noise and glitches in
// analog tape captures, but may have other uses as well.
//
// Author: antti.korhola@gmail.com
//
// License: Public domain. Credit would be nice, but do with this what you will.
//
// Forum thread: http://forum.doom9.org/showthread.php?t=170216
//               http://forum.videohelp.com/threads/362361-Median%28%29-plugin-for-AviSynth
//
// History:
// 12-Feb-2014,  0.1: Initial release. YUY2 support only
// 13-Feb-2014,  0.2: Added support for RGB and planar formats
// 13-Feb-2014,  0.3: Fixed output frame buffer issue
// 14-Feb-2014,  0.4: Added MedianBlend functionality
// 15-Mar-2014,  0.5: Added TemporalMedian functionality
// xx-Nov-2015,  0.6: Added sync functionality
//
//////////////////////////////////////////////////////////////////////////////

// Includes
#include "stdafx.h"


//////////////////////////////////////////////////////////////////////////////
// Create Median filter
//////////////////////////////////////////////////////////////////////////////
AVSValue __cdecl Create_Median(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	AVSValue array = args[0];
	int n = array.ArraySize();

    if (n < 3 || n > 25 || n % 2 == 0)
        env->ThrowError(ERROR_PREFIX "Need an odd number of clips between 3 and 25.");

	vector<PClip> clips;

	for (int i = 0; i < n; i++)
		clips.push_back(array[i].AsClip());

    // Parameters
    bool chroma = args[1].AsBool(true);
    int sync = args[2].AsInt(0);
    int samples = args[3].AsInt(4096U);
    bool debug = args[4].AsBool(false);

    // Validation
    if (sync < 0)
        env->ThrowError(ERROR_PREFIX "Sync needs to be a positive value.");

    if (samples < 0)
        env->ThrowError(ERROR_PREFIX "Samples needs to be a positive value.");

    // Set low and high so that a regular median function is achieved
    unsigned int limit = (n - 1) / 2;

	return new Median(clips[0], clips, limit, limit, false, chroma, sync, samples, debug, env);
}


//////////////////////////////////////////////////////////////////////////////
// Create TemporalMedian filter
//////////////////////////////////////////////////////////////////////////////
AVSValue __cdecl Create_TemporalMedian(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    vector<PClip> clips;
    clips.push_back(args[0].AsClip());

    // Parameters
    int radius = args[1].AsInt(1);
    bool chroma = args[2].AsBool(true);
    bool debug = args[3].AsBool(false);

    // Validation
    if (radius < 1 || radius > 12)
        env->ThrowError(ERROR_PREFIX "Radius needs to be between 1 and 12.");

    return new Median(clips[0], clips, radius, radius, true, chroma, 0, 0, debug, env);
}


//////////////////////////////////////////////////////////////////////////////
// Create MedianBlend filter
//////////////////////////////////////////////////////////////////////////////
AVSValue __cdecl Create_MedianBlend(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	AVSValue array = args[0];
	int n = array.ArraySize();

	if (n < 3 || n > 25)
		env->ThrowError(ERROR_PREFIX "Need 3-25 clips.");

	vector<PClip> clips;

	for (int i = 0; i < n; i++)
		clips.push_back(array[i].AsClip());

    // Parameters
    int low = args[1].AsInt(1);
    int high = args[2].AsInt(1);
    bool chroma = args[3].AsBool(true);
    int sync = args[4].AsInt(0);
    int samples = args[5].AsInt(4096U);
    bool debug = args[6].AsBool(false);

    // Validation
	if (low < 0 || high < 0 || low >= n || high >= n || low + high >= n)
		env->ThrowError(ERROR_PREFIX "Invalid values supplied for low and/or high limits.");

    if (sync < 0)
        env->ThrowError(ERROR_PREFIX "Sync needs to be a positive value.");

    if (samples < 0)
        env->ThrowError(ERROR_PREFIX "Samples needs to be a positive value.");

	return new Median(clips[0], clips, low, high, false, chroma, sync, samples, debug, env);
}


//////////////////////////////////////////////////////////////////////////////
// Add filters
//////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("Median", "c+[CHROMA]b[SYNC]i[SAMPLES]i[DEBUG]b", Create_Median, 0);
    env->AddFunction("TemporalMedian", "c[RADIUS]i[CHROMA]b[DEBUG]b", Create_TemporalMedian, 0);
	env->AddFunction("MedianBlend", "c+[LOW]i[HIGH]i[CHROMA]b[SYNC]i[SAMPLES]i[DEBUG]b", Create_MedianBlend, 0);

	return "Median of clips filter";
}
