/*
Copyright (c) 2012,2013,2014 Frank Bauernöppel

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, 
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
*/

This is a H.261 encoder, see http://www.itu.int/ITU-T/recommendations/rec.aspx?rec=1088.

The code can be compiled using gcc and many more plain old C compilers.

The code is written for readability and follows the H.261 standard. It is not optimized.

Raw YUV420p video in CIF (352 x 288) for input is found on https://media.xiph.org/video/derf/ 

Conversion from .y4m to .yuv was done using `ffmpeg -i football_cif.y4m football_cif.yuv`.


How to build
==
cmake -B build
cmake --build build

Tested with gcc version 11.4.0 and cmake version 3.31.2 on Ubuntu 22.04.5 LTS.


How to run
==
build/h261e


How to view
==
ffplay football_cif.h261 


Use the source, Luke.

Frank
