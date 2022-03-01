#ifndef MEDIAN_H
#define MEDIAN_H

#include <vector>
using std::vector;

const unsigned int MAX_DEPTH = 25;
const unsigned int MAX_OPT = 9;

//////////////////////////////////////////////////////////////////////////////
// Class definition
//////////////////////////////////////////////////////////////////////////////
class Median : public GenericVideoFilter
{
public:
    Median(PClip _child, vector<PClip> _clips, unsigned int _low, unsigned int _high, bool _temporal, bool _processchroma, unsigned int _sync, unsigned int _samples, bool _debug, IScriptEnvironment *env);
	~Median();

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
    vector<PClip> clips;
    unsigned int low;
    unsigned int high;
    bool temporal;
    bool processchroma;
    unsigned int sync;
    unsigned int samples;
    bool debug;

    unsigned int depth;
    unsigned int blend;
    bool fastprocess;
    vector<VideoInfo> info;

    unsigned char (*fastmedian)(unsigned char*);

    double CompareFrames(int plane, PVideoFrame a, PVideoFrame b, unsigned int points);
    void ProcessPlane(int plane, PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    void ProcessPlanarFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    void ProcessInterleavedFrame(PVideoFrame src[MAX_DEPTH], PVideoFrame& dst);
    inline unsigned char ProcessPixel(unsigned char* values) const;

    void debugf(const char* fmt, ...);

    unsigned int line;
    void textf(PVideoFrame& dst, const char* fmt, ...);
};


#endif // MEDIAN_H
