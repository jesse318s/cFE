/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/*
** File:
**    time_ut_cdc.c
**
** Purpose:
**    Unit tests for the Celestial Day Clock (CDC) module
**    (cfe_time_cdc.h / cfe_time_cdc.c).
**
** Coverage targets
**    - CFE_TIME_CDC_Init / CFE_TIME_CDC_SetBodyMaximums / CFE_TIME_CDC_GetBodyMaximums
**      round-trip for representative planet parameters
**    - tick / carry-propagation across seconds -> minutes -> hours boundaries
**    - wrap-around reset for clocks without fractional minutes (maxMinutes == 0)
**    - wrap-around reset for clocks with fractional minutes (maxMinutes != 0),
**      including the half-day transition and full-day reset
**    - military and standard time formatting outputs
**    - null-buffer and zero-length guards in the formatting functions
*/

#include "time_UT.h"
#include "cfe_time_cdc.h"

#include <string.h>

/* -----------------------------------------------------------------------
 * Helper: tick the clock N times without going through normal CFE services.
 * ----------------------------------------------------------------------- */
static void TickN(CFE_TIME_CDC_Clock_t *clock, int n)
{
    int i;
    for (i = 0; i < n; ++i)
        CFE_TIME_CDC_Tick(clock);
}

/* =======================================================================
 * Test_CDC_InitAndBodyMaximums
 *
 * Verify that Init zeros the time fields and that GetBodyMaximums inverts
 * SetBodyMaximums for a variety of planet configurations.
 * ======================================================================= */
void Test_CDC_InitAndBodyMaximums(void)
{
    CFE_TIME_CDC_Clock_t clock;
    int                  bm[2];

    UtPrintf("Begin Test CDC Init and Body Maximums");

    /* --- After Init all time fields must be zero --- */
    CFE_TIME_CDC_Init(&clock, 24, 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 0);

    /* --- Earth: 23h 56m --- round-trip GetBodyMaximums --- */
    CFE_TIME_CDC_Init(&clock,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].hours,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].minutes);
    CFE_TIME_CDC_GetBodyMaximums(&clock, bm);
    UtAssert_INT32_EQ(bm[0], CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].hours);
    UtAssert_INT32_EQ(bm[1], CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].minutes);

    /* --- Mars: 24h 37m --- round-trip GetBodyMaximums --- */
    CFE_TIME_CDC_Init(&clock,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_MARS].hours,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_MARS].minutes);
    CFE_TIME_CDC_GetBodyMaximums(&clock, bm);
    UtAssert_INT32_EQ(bm[0], CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_MARS].hours);
    UtAssert_INT32_EQ(bm[1], CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_MARS].minutes);

    /* --- Jupiter: 9h 55m --- round-trip GetBodyMaximums --- */
    CFE_TIME_CDC_Init(&clock,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_JUPITER].hours,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_JUPITER].minutes);
    CFE_TIME_CDC_GetBodyMaximums(&clock, bm);
    UtAssert_INT32_EQ(bm[0], CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_JUPITER].hours);
    UtAssert_INT32_EQ(bm[1], CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_JUPITER].minutes);

    /* --- Even hours, no minutes: 24h 0m --- */
    CFE_TIME_CDC_Init(&clock, 24, 0);
    CFE_TIME_CDC_GetBodyMaximums(&clock, bm);
    UtAssert_INT32_EQ(bm[0], 24);
    UtAssert_INT32_EQ(bm[1], 0);

    /* --- Hours below minimum are clamped to CFE_TIME_CDC_MAX_HOURS_MIN --- */
    CFE_TIME_CDC_Init(&clock, 1, 0);
    UtAssert_INT32_EQ(clock.maxHours, CFE_TIME_CDC_MAX_HOURS_MIN);
}

/* =======================================================================
 * Test_CDC_GettersSetters
 *
 * Verify individual field setters clamp correctly and getters return the
 * stored value.
 * ======================================================================= */
void Test_CDC_GettersSetters(void)
{
    CFE_TIME_CDC_Clock_t clock;

    UtPrintf("Begin Test CDC Getters and Setters");

    CFE_TIME_CDC_Init(&clock, 24, 0);

    /* Hours: within range */
    CFE_TIME_CDC_SetHours(&clock, 10);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 10);

    /* Hours: negative clamped to 0 */
    CFE_TIME_CDC_SetHours(&clock, -1);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 0);

    /* Hours: above maxHours clamped */
    CFE_TIME_CDC_SetHours(&clock, 999);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), clock.maxHours);

    /* MinutesDigit1: in range */
    CFE_TIME_CDC_SetMinutesDigit1(&clock, 3);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 3);

    /* MinutesDigit1: clamped to RADIX_MAX */
    CFE_TIME_CDC_SetMinutesDigit1(&clock, 99);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), CFE_TIME_CDC_RADIX_MAX);

    /* MinutesDigit2: in range */
    CFE_TIME_CDC_SetMinutesDigit2(&clock, 7);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 7);

    /* MinutesDigit2: clamped to SECONDARY_RADIX_MAX */
    CFE_TIME_CDC_SetMinutesDigit2(&clock, 99);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), CFE_TIME_CDC_SECONDARY_RADIX_MAX);

    /* SecondsDigit1 */
    CFE_TIME_CDC_SetSecondsDigit1(&clock, 2);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 2);

    CFE_TIME_CDC_SetSecondsDigit1(&clock, 99);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), CFE_TIME_CDC_RADIX_MAX);

    /* SecondsDigit2 */
    CFE_TIME_CDC_SetSecondsDigit2(&clock, 4);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 4);

    CFE_TIME_CDC_SetSecondsDigit2(&clock, 99);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), CFE_TIME_CDC_SECONDARY_RADIX_MAX);
}

/* =======================================================================
 * Test_CDC_TickCarry
 *
 * Verify carry-propagation: seconds -> minutes -> hours.
 * ======================================================================= */
void Test_CDC_TickCarry(void)
{
    CFE_TIME_CDC_Clock_t clock;

    UtPrintf("Begin Test CDC Tick and Carry");

    /* Use Earth (23h 56m) as a representative body */
    CFE_TIME_CDC_Init(&clock,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].hours,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].minutes);

    /* --- One tick advances secondsDigit2 --- */
    CFE_TIME_CDC_Tick(&clock);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 1);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 0);

    /* --- 9 more ticks -> secondsDigit2 wraps to 0, secondsDigit1 = 1 --- */
    TickN(&clock, 9);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 1);

    /* --- 50 more seconds (total 60) -> one full minute carried --- */
    TickN(&clock, 50);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 1);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 0);

    /* --- Advance 9 more minutes (540 s) -> minutesDigit2 wraps, minutesDigit1 = 1 --- */
    TickN(&clock, 9 * 60);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 1);

    /* --- Advance 50 more minutes -> one full hour carried --- */
    TickN(&clock, 50 * 60);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 1);
}

/* =======================================================================
 * Test_CDC_ResetNoMinutes
 *
 * Verify wrap-around reset for a clock with no fractional minutes
 * (maxMinutes == 0), e.g. a 24h even-hour day.
 * ======================================================================= */
void Test_CDC_ResetNoMinutes(void)
{
    CFE_TIME_CDC_Clock_t clock;
    bool                 wasReset;

    UtPrintf("Begin Test CDC Reset (maxMinutes == 0)");

    /* 24h 0m day */
    CFE_TIME_CDC_Init(&clock, 24, 0);
    UtAssert_INT32_EQ(clock.maxMinutes, 0);

    /* Advance to one second before the reset point:
     * hours = maxHours-1 = 23, minutes = 59, seconds = 59 */
    CFE_TIME_CDC_SetHours(&clock, 23);
    CFE_TIME_CDC_SetMinutesDigit1(&clock, 5);
    CFE_TIME_CDC_SetMinutesDigit2(&clock, 9);
    CFE_TIME_CDC_SetSecondsDigit1(&clock, 5);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, 9);

    /* CheckTimeReset at this state must return true and zero all fields */
    wasReset = CFE_TIME_CDC_CheckTimeReset(&clock);
    UtAssert_BOOL_TRUE(wasReset);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 0);

    /* Not at reset point: no reset expected */
    CFE_TIME_CDC_SetHours(&clock, 10);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, 5);
    wasReset = CFE_TIME_CDC_CheckTimeReset(&clock);
    UtAssert_BOOL_FALSE(wasReset);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 10);
}

/* =======================================================================
 * Test_CDC_ResetWithMinutes
 *
 * Verify wrap-around reset for a clock WITH fractional minutes
 * (maxMinutes != 0), covering both the half-day transition and the
 * full-day reset.
 * ======================================================================= */
void Test_CDC_ResetWithMinutes(void)
{
    CFE_TIME_CDC_Clock_t clock;
    bool                 wasReset;
    int                  halfHours;

    UtPrintf("Begin Test CDC Reset (maxMinutes != 0)");

    /* Earth: 23h 56m */
    CFE_TIME_CDC_Init(&clock,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].hours,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].minutes);
    UtAssert_INT32_EQ(clock.maxMinutes != 0, 1); /* must have fractional minutes */

    halfHours = clock.maxHours / 2;

    /* --- Half-day transition: clock at hours == halfHours, minutes at ceiling --- */
    CFE_TIME_CDC_SetHours(&clock, halfHours);
    /* maxMinutes - 1: the reset fires when minutesCombined >= maxMinutes - 1 */
    {
        int target = clock.maxMinutes - 1;
        CFE_TIME_CDC_SetMinutesDigit1(&clock, target / CFE_TIME_CDC_SECONDARY_RADIX);
        CFE_TIME_CDC_SetMinutesDigit2(&clock, target % CFE_TIME_CDC_SECONDARY_RADIX);
    }
    CFE_TIME_CDC_SetSecondsDigit1(&clock, CFE_TIME_CDC_RADIX_MAX);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, CFE_TIME_CDC_SECONDARY_RADIX_MAX);

    wasReset = CFE_TIME_CDC_CheckTimeReset(&clock);
    UtAssert_BOOL_TRUE(wasReset);
    /* After half-day reset, hours must advance to halfHours + 1 */
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), halfHours + 1);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 0);

    /* --- Full-day reset: hours >= maxHours, minutes at ceiling --- */
    CFE_TIME_CDC_SetHours(&clock, clock.maxHours);
    {
        int target = clock.maxMinutes - 1;
        CFE_TIME_CDC_SetMinutesDigit1(&clock, target / CFE_TIME_CDC_SECONDARY_RADIX);
        CFE_TIME_CDC_SetMinutesDigit2(&clock, target % CFE_TIME_CDC_SECONDARY_RADIX);
    }
    CFE_TIME_CDC_SetSecondsDigit1(&clock, CFE_TIME_CDC_RADIX_MAX);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, CFE_TIME_CDC_SECONDARY_RADIX_MAX);

    wasReset = CFE_TIME_CDC_CheckTimeReset(&clock);
    UtAssert_BOOL_TRUE(wasReset);
    /* After full-day reset everything returns to zero */
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetHours(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetMinutesDigit2(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit1(&clock), 0);
    UtAssert_INT32_EQ(CFE_TIME_CDC_GetSecondsDigit2(&clock), 0);

    /* --- No reset when not at ceiling --- */
    CFE_TIME_CDC_SetHours(&clock, 5);
    CFE_TIME_CDC_SetMinutesDigit1(&clock, 3);
    CFE_TIME_CDC_SetMinutesDigit2(&clock, 0);
    CFE_TIME_CDC_SetSecondsDigit1(&clock, 0);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, 0);
    wasReset = CFE_TIME_CDC_CheckTimeReset(&clock);
    UtAssert_BOOL_FALSE(wasReset);
}

/* =======================================================================
 * Test_CDC_Formatting
 *
 * Verify military and standard time string outputs, and the null/zero-
 * length buffer guards.
 * ======================================================================= */
void Test_CDC_Formatting(void)
{
    CFE_TIME_CDC_Clock_t clock;
    char                 milBuf[CFE_TIME_CDC_TIME_STR_LEN];
    char                 stdBuf[CFE_TIME_CDC_TIME_STR_LEN];
    char                 expectedMil[CFE_TIME_CDC_TIME_STR_LEN];
    char                 expectedStd[CFE_TIME_CDC_TIME_STR_LEN];

    UtPrintf("Begin Test CDC Formatting");

    /* Use a 24h even-hour clock so AM/PM boundary is at hour 12 */
    CFE_TIME_CDC_Init(&clock, 24, 0);

    /* Set time to 14:30:59 */
    CFE_TIME_CDC_SetHours(&clock, 14);
    CFE_TIME_CDC_SetMinutesDigit1(&clock, 3);
    CFE_TIME_CDC_SetMinutesDigit2(&clock, 0);
    CFE_TIME_CDC_SetSecondsDigit1(&clock, 5);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, 9);

    /* Military format: "14:30:59" */
    CFE_TIME_CDC_GetTimeMilitary(&clock, milBuf, (int)sizeof(milBuf));
    strcpy(expectedMil, "14:30:59");
    UtAssert_STRINGBUF_EQ(milBuf, sizeof(milBuf), expectedMil, sizeof(expectedMil));

    /* Standard format: "2:30:59 PM" (14 - 12 = 2, PM) */
    CFE_TIME_CDC_GetTime(&clock, stdBuf, (int)sizeof(stdBuf));
    strcpy(expectedStd, "2:30:59 PM");
    UtAssert_STRINGBUF_EQ(stdBuf, sizeof(stdBuf), expectedStd, sizeof(expectedStd));

    /* AM time: hour 6 */
    CFE_TIME_CDC_SetHours(&clock, 6);
    CFE_TIME_CDC_GetTime(&clock, stdBuf, (int)sizeof(stdBuf));
    strcpy(expectedStd, "6:30:59 AM");
    UtAssert_STRINGBUF_EQ(stdBuf, sizeof(stdBuf), expectedStd, sizeof(expectedStd));

    /* GetTimes fills both buffers at once */
    CFE_TIME_CDC_SetHours(&clock, 14);
    CFE_TIME_CDC_GetTimes(&clock, milBuf, (int)sizeof(milBuf), stdBuf, (int)sizeof(stdBuf));
    strcpy(expectedMil, "14:30:59");
    strcpy(expectedStd, "2:30:59 PM");
    UtAssert_STRINGBUF_EQ(milBuf, sizeof(milBuf), expectedMil, sizeof(expectedMil));
    UtAssert_STRINGBUF_EQ(stdBuf, sizeof(stdBuf), expectedStd, sizeof(expectedStd));

    /* --- Null buffer guard: must not crash --- */
    UtAssert_VOIDCALL(CFE_TIME_CDC_GetTimeMilitary(&clock, NULL, (int)sizeof(milBuf)));
    UtAssert_VOIDCALL(CFE_TIME_CDC_GetTime(&clock, NULL, (int)sizeof(stdBuf)));
    UtAssert_VOIDCALL(CFE_TIME_CDC_GetMeridiemIndicator(&clock, NULL, 4));

    /* --- Zero-length buffer guard: must not crash --- */
    UtAssert_VOIDCALL(CFE_TIME_CDC_GetTimeMilitary(&clock, milBuf, 0));
    UtAssert_VOIDCALL(CFE_TIME_CDC_GetTime(&clock, stdBuf, 0));
    UtAssert_VOIDCALL(CFE_TIME_CDC_GetMeridiemIndicator(&clock, stdBuf, 0));

    /*
     * --- Fractional-minute body (maxMinutes != 0) ---
     *
     * Earth (23h 56m): after normalization maxHours = 23, maxMinutes = 58.
     * Half-day index = maxHours / 2 = 11.
     * At hours = 14 (> 11), GetStandardHours takes the fractional-minute
     * branch: 14 - 11 - 1 = 2.  GetMeridiemIndicator takes its
     * maxMinutes != 0 PM branch.
     *
     * This exercises different code paths in both helpers compared to the
     * maxMinutes == 0 tests above.
     */
    CFE_TIME_CDC_Init(&clock,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].hours,
                      CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_EARTH].minutes);
    CFE_TIME_CDC_SetHours(&clock, 14);
    CFE_TIME_CDC_SetMinutesDigit1(&clock, 3);
    CFE_TIME_CDC_SetMinutesDigit2(&clock, 0);
    CFE_TIME_CDC_SetSecondsDigit1(&clock, 5);
    CFE_TIME_CDC_SetSecondsDigit2(&clock, 9);

    /* PM case (hours > maxHours/2 = 11): standard hours = 14 - 11 - 1 = 2 */
    CFE_TIME_CDC_GetTimes(&clock, milBuf, (int)sizeof(milBuf), stdBuf, (int)sizeof(stdBuf));
    strcpy(expectedMil, "14:30:59");
    strcpy(expectedStd, "2:30:59 PM");
    UtAssert_STRINGBUF_EQ(milBuf, sizeof(milBuf), expectedMil, sizeof(expectedMil));
    UtAssert_STRINGBUF_EQ(stdBuf, sizeof(stdBuf), expectedStd, sizeof(expectedStd));

    /* AM case (hours <= maxHours/2 = 11): GetMeridiemIndicator ante-meridiem branch */
    CFE_TIME_CDC_SetHours(&clock, 8);
    CFE_TIME_CDC_GetTime(&clock, stdBuf, (int)sizeof(stdBuf));
    strcpy(expectedStd, "8:30:59 AM");
    UtAssert_STRINGBUF_EQ(stdBuf, sizeof(stdBuf), expectedStd, sizeof(expectedStd));
}
