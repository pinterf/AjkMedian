# AjkMedian
An Avisynth Median Filter by ajk

## Links

Forum thread: http://forum.doom9.org/showthread.php?t=170216

Forum thread: http://forum.videohelp.com/threads/362361-Median%28%29-plugin-for-AviSynth

https://forum.doom9.org/showthread.php?p=1864302#post1864302

https://forum.doom9.org/showpost.php?p=1864406&postcount=55

Vapoursynth port https://github.com/dubhater/vapoursynth-median

## Change log

20220301 v0.7 (pinterf)
  - move to github: https://github.com/pinterf/AjkMedian
  - add README.md, build
  - add Window version resource to DLL
  - Update Avisynth headers
  - pass frame properties
  - move to VS2019 (v142 toolset)
  - add CMake build environment
  - Linux/GCC friendly source
  - DLL/so name is changed to AjkMedian/libajkpedian (from simple Median - possible name collisions)

20190201 v0.6 (TomArrow)
  - https://forum.doom9.org/showthread.php?p=1864406#post1864406
  - Support RGB64 (but not the fast processing mode that's supported in the 8-bit color spaces)
  - Implement the new AviSynth+ API (V6)
  - Have both 32 bit and 64 bit platforms set up for VS 2017 (not sure if backwards compatible for older VS versions, but maybe?)

201511xx 0.6 (ajk)
  - Added sync functionality

20140215 0.5 (ajk)
  - the plugin will accept between 3 and 25 clips
  - chroma processing can be turned off with "chroma=false", or in the case of RGB32 this will turn off processing for the alpha channel
  - there is also a more configurable MedianBlend() function, see examples in the readme or posts further below
  - Added TemporalMedian functionality 
    https://forum.doom9.org/showpost.php?p=1667483&postcount=1

20140214 0.4 (ajk)
  - Added MedianBlend functionality

20140213 0.3 (ajk)
  - Added support for RGB and planar formats

20140212 0.1 (ajk)
  - Initial release. YUY2 support only

## Build Instructrions

### Windows MSVC

* build from IDE

## Windows GCC
(mingw installed by msys2)
From the 'build' folder under project root:

    del ..\CMakeCache.txt
    cmake .. -G "MinGW Makefiles"
    cmake --build . --config Release  

## Linux build instructions

* Clone repo

        git clone https://github.com/pinterf/AjkMedian
        cd AjkMedian
        cmake -B build -S .
        cmake --build build

  Useful hints:        
  build after clean:

      cmake --build build --clean-first

  delete CMake cache

      rm build/CMakeCache.txt

* Find binaries at

        build/AjkMedian/AjkMedian.so

* Install binaries

        cd build
        sudo make install

