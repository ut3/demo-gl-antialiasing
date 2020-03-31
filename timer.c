#include <time.h>
#include <stdio.h>
#include "Timer.h"

#ifdef WINDOWS
#include <Windows.h>

static unsigned int g_PreviousTick;
static unsigned int g_FrameCount;
static unsigned int g_LastSecondTick;
static float g_TimeDelta;

void InitialiseTimer()
{
	g_PreviousTick = GetTickCount();
	g_LastSecondTick = g_PreviousTick;
	g_FrameCount = 0;
	g_TimeDelta = 0.0f;
}

unsigned char ProcessTimer(unsigned int *framespersecond)
{
	unsigned char retval = 0;
	unsigned int tick = GetTickCount();

	*framespersecond = g_FrameCount;
	g_TimeDelta = (tick - g_PreviousTick) / 1000.0f;

	if (tick - g_LastSecondTick >= 1000)
	{
		retval = 1;
		g_FrameCount = 0;
		g_LastSecondTick = tick;
	}
	g_PreviousTick = tick;
	g_FrameCount++;

	return retval;
}

float GetPreviousFrameDeltaInSeconds()
{
	return g_TimeDelta;
}

#else

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

#endif
