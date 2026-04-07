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
** File: cfe_time_cdc.c
**
** Purpose:  cFE Time Services Celestial Day Clock implementation
**
** Notes:    C translation of the CelestialDayClock C++ class.  All logic
**           and behavior from the original are preserved exactly, including
**           the setBodyMaximums normalization algorithm, the wrap-around
**           reset logic, sexagesimal tick/carry, and AM/PM formatting.
**           No dynamic memory allocation is used.
*/

#include "cfe_time_cdc.h"

#include <stdio.h>

/*
** Planet sidereal day length lookup table (hours, minutes).
** Indices correspond to CFE_TIME_CDC_Planet_t enum values.
*/
const CFE_TIME_CDC_PlanetDay_t CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_PLANET_COUNT] =
{
    [CFE_TIME_CDC_MERCURY] = { 1407, 36 },
    [CFE_TIME_CDC_VENUS]   = { 5832, 36 },
    [CFE_TIME_CDC_EARTH]   = {   23, 56 },
    [CFE_TIME_CDC_MARS]    = {   24, 37 },
    [CFE_TIME_CDC_JUPITER] = {    9, 55 },
    [CFE_TIME_CDC_SATURN]  = {   10, 39 },
    [CFE_TIME_CDC_URANUS]  = {   17, 14 },
    [CFE_TIME_CDC_NEPTUNE] = {   16,  6 },
};

/*
** File-scope constant derived from the two radix #defines.
**
** halfMaxBodyMinutes = (RADIX_MAX / 2 * SECONDARY_RADIX + SECONDARY_RADIX_MAX) - 1
**                    = (5/2 * 10 + 9) - 1  =  28
**
** This mirrors the constexpr in setBodyMaximums and caps the normalized
** half-day minutes to the highest value representable by one tens digit
** (0-5) and one units digit (0-9) in sexagesimal notation, minus one.
*/
static const int CFE_TIME_CDC_HALF_MAX_BODY_MINUTES =
    (CFE_TIME_CDC_RADIX_MAX / 2 * CFE_TIME_CDC_SECONDARY_RADIX + CFE_TIME_CDC_SECONDARY_RADIX_MAX) - 1;

/*----------------------------------------------------------------
 * Static helper: CFE_TIME_CDC_Clamp
 *
 * Clamp value to [0, max].  Mirrors the private clamp() method.
 *----------------------------------------------------------------*/
static int CFE_TIME_CDC_Clamp(int value, int max)
{
    if (value > max) return max;
    if (value < 0)   return 0;

    return value;
}

/*----------------------------------------------------------------
 * Static helper: CFE_TIME_CDC_TickMinutes
 *
 * Carry-propagate a one-minute increment through the two minute
 * digit fields and into the hours field.  Mirrors the private
 * tickMinutes() method.
 *----------------------------------------------------------------*/
static void CFE_TIME_CDC_TickMinutes(CFE_TIME_CDC_Clock_t *clock)
{
    ++clock->minutesDigit2;

    if (clock->minutesDigit2 >= CFE_TIME_CDC_SECONDARY_RADIX)
    {
        clock->minutesDigit2 = 0;
        ++clock->minutesDigit1;

        if (clock->minutesDigit1 >= CFE_TIME_CDC_RADIX)
        {
            clock->minutesDigit1 = 0;
            ++clock->hours;
        }
    }
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_Init
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_Init(CFE_TIME_CDC_Clock_t *clock, int h, int m)
{
    if (clock == NULL)
        return;

    clock->maxHours      = 0;
    clock->maxMinutes    = 0;
    clock->hours         = 0;
    clock->minutesDigit1 = 0;
    clock->minutesDigit2 = 0;
    clock->secondsDigit1 = 0;
    clock->secondsDigit2 = 0;

    CFE_TIME_CDC_SetBodyMaximums(clock, h, m);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_SetHours
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_SetHours(CFE_TIME_CDC_Clock_t *clock, int h)
{
    if (clock == NULL)
        return;

    clock->hours = CFE_TIME_CDC_Clamp(h, clock->maxHours);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetHours
 *----------------------------------------------------------------*/
int CFE_TIME_CDC_GetHours(const CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return 0;

    return clock->hours;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_SetMinutesDigit1
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_SetMinutesDigit1(CFE_TIME_CDC_Clock_t *clock, int m)
{
    if (clock == NULL)
        return;

    clock->minutesDigit1 = CFE_TIME_CDC_Clamp(m, CFE_TIME_CDC_RADIX_MAX);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetMinutesDigit1
 *----------------------------------------------------------------*/
int CFE_TIME_CDC_GetMinutesDigit1(const CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return 0;

    return clock->minutesDigit1;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_SetMinutesDigit2
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_SetMinutesDigit2(CFE_TIME_CDC_Clock_t *clock, int m)
{
    if (clock == NULL)
        return;

    clock->minutesDigit2 = CFE_TIME_CDC_Clamp(m, CFE_TIME_CDC_SECONDARY_RADIX_MAX);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetMinutesDigit2
 *----------------------------------------------------------------*/
int CFE_TIME_CDC_GetMinutesDigit2(const CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return 0;

    return clock->minutesDigit2;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_SetSecondsDigit1
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_SetSecondsDigit1(CFE_TIME_CDC_Clock_t *clock, int s)
{
    if (clock == NULL)
        return;

    clock->secondsDigit1 = CFE_TIME_CDC_Clamp(s, CFE_TIME_CDC_RADIX_MAX);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetSecondsDigit1
 *----------------------------------------------------------------*/
int CFE_TIME_CDC_GetSecondsDigit1(const CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return 0;

    return clock->secondsDigit1;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_SetSecondsDigit2
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_SetSecondsDigit2(CFE_TIME_CDC_Clock_t *clock, int s)
{
    if (clock == NULL)
        return;

    clock->secondsDigit2 = CFE_TIME_CDC_Clamp(s, CFE_TIME_CDC_SECONDARY_RADIX_MAX);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetSecondsDigit2
 *----------------------------------------------------------------*/
int CFE_TIME_CDC_GetSecondsDigit2(const CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return 0;

    return clock->secondsDigit2;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_SetBodyMaximums
 *
 * Normalize the caller-supplied (h, m) body-day length into the
 * internal (maxHours, maxMinutes) representation used by the tick
 * and reset logic.
 *
 * Algorithm (mirrors C++ setBodyMaximums verbatim):
 *   1. Enforce minimum hours floor.
 *   2. Halve the minutes to get the half-day minutes count.
 *   3. Force the half-day count to be even and cap it.
 *   4. If maxHours is odd, absorb the extra half-hour into maxMinutes.
 *   5. If any fractional minutes remain, add one whole hour to maxHours.
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_SetBodyMaximums(CFE_TIME_CDC_Clock_t *clock, int h, int m)
{
    if (clock == NULL)
        return;

    if (h < CFE_TIME_CDC_MAX_HOURS_MIN)
        h = CFE_TIME_CDC_MAX_HOURS_MIN;

    clock->maxHours = h;

    /* Convert full-day minutes to half-day minutes */
    m = m / 2;

    if (m < 0)
        m = 0;

    /* Ensure the half-day count is even so both halves are symmetric */
    if (m % 2 == 1)
        --m;

    if (m > CFE_TIME_CDC_HALF_MAX_BODY_MINUTES)
        m = CFE_TIME_CDC_HALF_MAX_BODY_MINUTES;

    clock->maxMinutes = m;

    /* Absorb an odd maxHours into the minutes so the day splits cleanly */
    if (clock->maxHours % 2 == 1)
    {
        --clock->maxHours;
        clock->maxMinutes += CFE_TIME_CDC_RADIX * CFE_TIME_CDC_SECONDARY_RADIX / 2;
    }

    /* A non-zero minutes remainder needs one additional hour slot */
    if (clock->maxMinutes > 0)
        ++clock->maxHours;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetBodyMaximums
 *
 * Reconstruct the user-visible (bodyMaxHours, bodyMaxMinutes) from
 * the normalized internal state.  Mirrors C++ getBodyMaximums().
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_GetBodyMaximums(const CFE_TIME_CDC_Clock_t *clock, int bodyMaximums[2])
{
    if (clock == NULL || bodyMaximums == NULL)
        return;

    /* A truly odd maxHours occurs only when maxMinutes absorbed the extra
       half-hour (i.e. maxMinutes >= radix * secondaryRadix / 2 = 30). */
    const bool hasTrulyOddMaxHours = (clock->maxHours % 2 == 1) &&
                                     (clock->maxMinutes >= CFE_TIME_CDC_RADIX *
                                                           CFE_TIME_CDC_SECONDARY_RADIX / 2);

    int bodyMaxHours;
    int bodyMaxMinutes;

    if (hasTrulyOddMaxHours || clock->maxMinutes == 0)
        bodyMaxHours = clock->maxHours;
    else
        bodyMaxHours = clock->maxHours - 1;

    if (bodyMaxHours % 2 == 1)
        bodyMaxMinutes = (clock->maxMinutes - CFE_TIME_CDC_RADIX * CFE_TIME_CDC_SECONDARY_RADIX / 2) * 2;
    else
        bodyMaxMinutes = clock->maxMinutes * 2;

    bodyMaximums[0] = bodyMaxHours;
    bodyMaximums[1] = bodyMaxMinutes;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetTimeMilitary
 *
 * Format: "H:MM:SS" where H is unpadded hours and MM, SS are the
 * individual minute/second digit fields concatenated in pairs,
 * e.g. "14:30:59" for 14 h, 30 min, 59 sec.
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_GetTimeMilitary(const CFE_TIME_CDC_Clock_t *clock, char *buf, int bufLen)
{
    if (buf == NULL || bufLen <= 0)
        return;

    snprintf(buf, (size_t)bufLen, "%d%c%d%d%c%d%d",
             clock->hours,
             CFE_TIME_CDC_DELIMITER,
             clock->minutesDigit1, clock->minutesDigit2,
             CFE_TIME_CDC_DELIMITER,
             clock->secondsDigit1, clock->secondsDigit2);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetStandardHours
 *
 * Convert the current hours field to its 12-hour equivalent.
 * Mirrors C++ getStandardHours() case-for-case.
 *----------------------------------------------------------------*/
int CFE_TIME_CDC_GetStandardHours(const CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return 0;

    /* Midnight on an exact-hour boundary: display the half-day maximum */
    if (clock->maxMinutes == 0 && clock->hours == 0)
        return clock->maxHours / 2;

    /* Roll-over sentinel (hours == maxHours) maps back to 0 */
    if (clock->maxMinutes == 0 && clock->hours == clock->maxHours)
        return 0;

    /* Second half of the day when minutes are absent */
    if (clock->maxMinutes == 0 && clock->hours > clock->maxHours / 2)
        return clock->hours - clock->maxHours / 2;

    /* Second half of the day when fractional minutes are present */
    if (clock->maxMinutes != 0 && clock->hours > clock->maxHours / 2)
        return clock->hours - clock->maxHours / 2 - 1;

    return clock->hours;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetMeridiemIndicator
 *
 * Write " AM" or " PM" (three characters plus NUL) into buf.
 * Mirrors the C++ getMeridiemIndicator() logic.
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_GetMeridiemIndicator(const CFE_TIME_CDC_Clock_t *clock, char *buf, int bufLen)
{
    if (clock == NULL || buf == NULL || bufLen <= 0)
        return;

    /* Post-meridiem: exact-hour boundary, second half (excluding midnight) */
    if (clock->maxMinutes == 0 &&
        clock->hours >= clock->maxHours / 2 &&
        clock->hours != clock->maxHours)
    {
        snprintf(buf, (size_t)bufLen, " %c%c", CFE_TIME_CDC_POST_CHAR, CFE_TIME_CDC_MERIDIEM_CHAR);
        return;
    }

    /* Post-meridiem: fractional-minute boundary, strictly past the midpoint */
    if (clock->maxMinutes != 0 && clock->hours > clock->maxHours / 2)
    {
        snprintf(buf, (size_t)bufLen, " %c%c", CFE_TIME_CDC_POST_CHAR, CFE_TIME_CDC_MERIDIEM_CHAR);
        return;
    }

    snprintf(buf, (size_t)bufLen, " %c%c", CFE_TIME_CDC_ANTE_CHAR, CFE_TIME_CDC_MERIDIEM_CHAR);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetTime
 *
 * Format: "H:MM:SS XM", e.g. "7:30:59 PM".
 * Mirrors C++ getTime().
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_GetTime(const CFE_TIME_CDC_Clock_t *clock, char *buf, int bufLen)
{
    /* 4 bytes: space + letter + 'M' + NUL */
    char meridiemBuf[4];

    if (buf == NULL || bufLen <= 0)
        return;

    CFE_TIME_CDC_GetMeridiemIndicator(clock, meridiemBuf, (int)sizeof(meridiemBuf));

    snprintf(buf, (size_t)bufLen, "%d%c%d%d%c%d%d%s",
             CFE_TIME_CDC_GetStandardHours(clock),
             CFE_TIME_CDC_DELIMITER,
             clock->minutesDigit1, clock->minutesDigit2,
             CFE_TIME_CDC_DELIMITER,
             clock->secondsDigit1, clock->secondsDigit2,
             meridiemBuf);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_GetTimes
 *
 * Fill both the military and standard time string buffers.
 * Mirrors C++ getTimes().
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_GetTimes(const CFE_TIME_CDC_Clock_t *clock,
                           char *military, int milLen,
                           char *standard, int stdLen)
{
    CFE_TIME_CDC_GetTimeMilitary(clock, military, milLen);
    CFE_TIME_CDC_GetTime(clock, standard, stdLen);
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_CheckTimeReset
 *
 * Evaluate the wrap-around condition and reset the clock if met.
 * Two mutually exclusive cases (maxMinutes == 0 and != 0) mirror
 * the original C++ logic exactly.
 *
 * Returns true if the clock was reset.
 *----------------------------------------------------------------*/
bool CFE_TIME_CDC_CheckTimeReset(CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return false;

    /* All hours and all minutes are at their ceiling values */
    const bool areHoursMax = (clock->hours >= clock->maxHours - 1) &&
                             (clock->minutesDigit1 == CFE_TIME_CDC_RADIX_MAX) &&
                             (clock->minutesDigit2 == CFE_TIME_CDC_SECONDARY_RADIX_MAX);

    /* Minutes have reached the fractional maximum at the half-day or full-day point */
    const bool areMinutesMax =
        (clock->hours == clock->maxHours / 2 || clock->hours >= clock->maxHours) &&
        (clock->minutesDigit1 * CFE_TIME_CDC_SECONDARY_RADIX + clock->minutesDigit2 >=
         clock->maxMinutes - 1);

    /* Both second digits are at their ceiling */
    const bool areSecondsMax = (clock->secondsDigit1 == CFE_TIME_CDC_RADIX_MAX) &&
                               (clock->secondsDigit2 == CFE_TIME_CDC_SECONDARY_RADIX_MAX);

    bool isReset = false;

    /* Case 1: no fractional minutes -- reset everything to zero on hour overflow */
    if (clock->maxMinutes == 0 && areHoursMax && areSecondsMax)
    {
        clock->hours         = 0;
        clock->minutesDigit1 = 0;
        clock->minutesDigit2 = 0;
        clock->secondsDigit1 = 0;
        clock->secondsDigit2 = 0;
        isReset = true;
    }

    /* Case 2: fractional minutes present -- reset at the half-day or full-day
       minutes boundary; advance to the post-meridiem start if at mid-day */
    if (clock->maxMinutes != 0 && areMinutesMax && areSecondsMax)
    {
        clock->hours = (clock->hours == clock->maxHours / 2)
                           ? clock->maxHours / 2 + 1
                           : 0;
        clock->minutesDigit1 = 0;
        clock->minutesDigit2 = 0;
        clock->secondsDigit1 = 0;
        clock->secondsDigit2 = 0;
        isReset = true;
    }

    return isReset;
}

/*----------------------------------------------------------------
 * CFE_TIME_CDC_Tick
 *
 * Advance the clock by one second, carrying through the digit
 * fields in the order: secondsDigit2 -> secondsDigit1 -> minutes.
 * Mirrors C++ tick().
 *----------------------------------------------------------------*/
void CFE_TIME_CDC_Tick(CFE_TIME_CDC_Clock_t *clock)
{
    if (clock == NULL)
        return;

    if (CFE_TIME_CDC_CheckTimeReset(clock))
        return;

    ++clock->secondsDigit2;

    if (clock->secondsDigit2 >= CFE_TIME_CDC_SECONDARY_RADIX)
    {
        clock->secondsDigit2 = 0;
        ++clock->secondsDigit1;

        if (clock->secondsDigit1 >= CFE_TIME_CDC_RADIX)
        {
            clock->secondsDigit1 = 0;
            CFE_TIME_CDC_TickMinutes(clock);
        }
    }
}
