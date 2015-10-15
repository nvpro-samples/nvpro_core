/*-----------------------------------------------------------------------
    Copyright (c) 2013, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------

//-----------------------------------------------------------------------------
// TimeSampler work
//-----------------------------------------------------------------------------
struct TimeSampler
{
    bool                bNonStopRendering;
    int                 renderCnt;
#ifdef WIN32
    LARGE_INTEGER       start_time, end_time, freq;
#endif
    int                 timing_counter;
    int                 maxTimeSamples;
    int                 frameFPS;
    double              frameDT;
    double              timeSamplingFreq;
    TimeSampler()
    {
        bNonStopRendering = true;
        renderCnt = 1;
        timing_counter = 0;
        maxTimeSamples = 60;
        frameDT = 1.0/60.0;
        frameFPS = 0;
        timeSamplingFreq = 0.1;
#ifdef WIN32
        QueryPerformanceCounter(&start_time);
        QueryPerformanceCounter(&end_time);
#endif
    }
    inline double   getTiming() { return frameDT; }
    inline int      getFPS() { return frameFPS; }
    void            setTimeSamplingFreq(float s) { timeSamplingFreq = (double)s; }
    void            resetSampling(int i=10) { maxTimeSamples = i; }
    bool update(bool bContinueToRender)
    {
#ifdef WIN32
        bool updated = false;
        int totalSamples;
        totalSamples = (bContinueToRender || bNonStopRendering) ? maxTimeSamples : timing_counter;

        // avoid extreme situations
	    #define MINDT (1.0/2000.0)
	    if(frameDT < MINDT)
        {
	    	frameDT = MINDT;
            totalSamples = timing_counter;
        }

        if((timing_counter >= totalSamples) && (totalSamples > 0))
        {
            timing_counter = 0;
            QueryPerformanceCounter(&end_time);
            QueryPerformanceFrequency(&freq);
            frameDT = (((double)end_time.QuadPart) - ((double)start_time.QuadPart))/((double)freq.QuadPart);
            frameDT /= totalSamples;
			//#define MINDT (1.0/60.0)
			//#define MAXDT (1.0/3000.0)
			//if(frameDT < MINDT)
			//	frameDT = MINDT;
			//else if(frameDT > MAXDT)
			//	frameDT = MAXDT;
            frameFPS = (int)(1.0/frameDT);
            // update the amount of samples to average, depending on the speed of the scene
            maxTimeSamples = (int)(0.15/(frameDT));
            if(maxTimeSamples == 0)
                maxTimeSamples = 10; // just to avoid 0...
            //else if(maxTimeSamples > 200)
            //    maxTimeSamples = 200;
            updated = true;
        }
        if(bContinueToRender || bNonStopRendering)
        {
            if(timing_counter == 0)
                QueryPerformanceCounter(&start_time);
            timing_counter++;
        }
        return updated;
#else
	// Linux/OSX etc. TODO
	return true;
#endif
    }
};
