#include <time.h>
#include <stdio.h>
#include "Timer.h"

static time_t g_PreviousTimeValue;
static unsigned int g_FrameCount;
static unsigned int g_PreviousFPS;

void InitialiseTimer()
{
	g_PreviousTimeValue = time(NULL);
	g_FrameCount = 0;
	g_PreviousFPS = 0;
}

unsigned char ProcessTimer(unsigned int *framespersecond)
{
	unsigned char retval = 0;
	time_t tick = time(NULL);

	*framespersecond = g_FrameCount;

	if (tick - g_PreviousTimeValue >= 1)
	{
		retval = 1;
		g_PreviousFPS = g_FrameCount;
		g_FrameCount = 0;
		g_PreviousTimeValue = tick;
	}
	g_FrameCount++;

	return retval;
}

float GetPreviousFrameDeltaInSeconds()
{
	if (g_PreviousFPS == 0)
	{
		return 0.01f;
	}
	else
	{
		return 1.0f / (float)g_PreviousFPS;
	}
}
