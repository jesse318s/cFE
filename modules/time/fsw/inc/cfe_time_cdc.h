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

/**
 * @file
 *
 *  cFE Time Services Celestial Day Clock
 */

#ifndef CFE_TIME_CDC_H
#define CFE_TIME_CDC_H

#include <stdbool.h>
#include <stdint.h>

/*
** Constants
*/

/** Tens-digit radix for minutes and seconds fields (range 0-5) */
#define CFE_TIME_CDC_RADIX               6

/** Units-digit radix for minutes and seconds fields (range 0-9) */
#define CFE_TIME_CDC_SECONDARY_RADIX     10

/** Maximum value of the tens digit (radix - 1) */
#define CFE_TIME_CDC_RADIX_MAX           (CFE_TIME_CDC_RADIX - 1)

/** Maximum value of the units digit (secondaryRadix - 1) */
#define CFE_TIME_CDC_SECONDARY_RADIX_MAX (CFE_TIME_CDC_SECONDARY_RADIX - 1)

/** Minimum value accepted for maxHours during initialization */
#define CFE_TIME_CDC_MAX_HOURS_MIN       2

/**
 * Size of a caller-supplied time string buffer, large enough for any
 * supported planet.  Venus has the largest maxHours (5833 after
 * normalization), which produces a 14-byte standard string
 * "2916:59:59 PM\0"; 16 provides a safe margin.
 */
#define CFE_TIME_CDC_TIME_STR_LEN        16

/** Field delimiter character used in time strings */
#define CFE_TIME_CDC_DELIMITER           ':'

/** First character of the ante-meridiem indicator ('A') */
#define CFE_TIME_CDC_ANTE_CHAR           'A'

/** First character of the post-meridiem indicator ('P') */
#define CFE_TIME_CDC_POST_CHAR           'P'

/** Second character of the meridiem indicator ('M') */
#define CFE_TIME_CDC_MERIDIEM_CHAR       'M'

/*
** Type definitions
*/

/**
 * @brief Celestial Day Clock state.
 *
 * All fields are plain ints; no dynamic allocation.  Initialize with
 * CFE_TIME_CDC_Init before use.
 */
typedef struct
{
    int maxHours;       /**< Normalized internal maximum hours */
    int maxMinutes;     /**< Normalized internal maximum minutes */
    int hours;          /**< Current hour field */
    int minutesDigit1;  /**< Tens digit of current minutes (0-5) */
    int minutesDigit2;  /**< Units digit of current minutes (0-9) */
    int secondsDigit1;  /**< Tens digit of current seconds (0-5) */
    int secondsDigit2;  /**< Units digit of current seconds (0-9) */
} CFE_TIME_CDC_Clock_t;

/**
 * @brief Sidereal day length for a celestial body, expressed as
 *        integer hours and integer minutes.
 */
typedef struct
{
    int hours;
    int minutes;
} CFE_TIME_CDC_PlanetDay_t;

/**
 * @brief Index enumeration for CFE_TIME_CDC_PlanetDayLengths[].
 */
typedef enum
{
    CFE_TIME_CDC_MERCURY = 0,
    CFE_TIME_CDC_VENUS,
    CFE_TIME_CDC_EARTH,
    CFE_TIME_CDC_MARS,
    CFE_TIME_CDC_JUPITER,
    CFE_TIME_CDC_SATURN,
    CFE_TIME_CDC_URANUS,
    CFE_TIME_CDC_NEPTUNE,
    CFE_TIME_CDC_PLANET_COUNT /**< Sentinel -- number of entries in the table */
} CFE_TIME_CDC_Planet_t;

/**
 * @brief Read-only table of sidereal day lengths for all eight solar-system
 *        planets.  Pass an entry's fields to CFE_TIME_CDC_Init to create a
 *        planet-specific clock.
 */
extern const CFE_TIME_CDC_PlanetDay_t CFE_TIME_CDC_PlanetDayLengths[CFE_TIME_CDC_PLANET_COUNT];

/*
** Function declarations
*/

/**
 * @brief Initialize a Celestial Day Clock.
 *
 * Zeros all time fields and then applies CFE_TIME_CDC_SetBodyMaximums with
 * the supplied parameters.  Must be called before any other function.
 *
 * @param clock  Pointer to uninitialized clock struct.
 * @param h      Desired maximum hours for the celestial body's day.
 * @param m      Desired maximum minutes for the celestial body's day.
 */
void CFE_TIME_CDC_Init(CFE_TIME_CDC_Clock_t *clock, int h, int m);

/**
 * @brief Set the hours field, clamping to [0, maxHours].
 *
 * @param clock  Pointer to clock struct.
 * @param h      New hours value.
 */
void CFE_TIME_CDC_SetHours(CFE_TIME_CDC_Clock_t *clock, int h);

/**
 * @brief Return the current hours field.
 *
 * @param clock  Pointer to clock struct.
 * @return Current hours value.
 */
int CFE_TIME_CDC_GetHours(const CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Set the tens digit of minutes, clamping to [0, CFE_TIME_CDC_RADIX_MAX].
 *
 * @param clock  Pointer to clock struct.
 * @param m      New value for minutesDigit1.
 */
void CFE_TIME_CDC_SetMinutesDigit1(CFE_TIME_CDC_Clock_t *clock, int m);

/**
 * @brief Return the tens digit of the current minutes field.
 *
 * @param clock  Pointer to clock struct.
 * @return Current minutesDigit1 value.
 */
int CFE_TIME_CDC_GetMinutesDigit1(const CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Set the units digit of minutes, clamping to [0, CFE_TIME_CDC_SECONDARY_RADIX_MAX].
 *
 * @param clock  Pointer to clock struct.
 * @param m      New value for minutesDigit2.
 */
void CFE_TIME_CDC_SetMinutesDigit2(CFE_TIME_CDC_Clock_t *clock, int m);

/**
 * @brief Return the units digit of the current minutes field.
 *
 * @param clock  Pointer to clock struct.
 * @return Current minutesDigit2 value.
 */
int CFE_TIME_CDC_GetMinutesDigit2(const CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Set the tens digit of seconds, clamping to [0, CFE_TIME_CDC_RADIX_MAX].
 *
 * @param clock  Pointer to clock struct.
 * @param s      New value for secondsDigit1.
 */
void CFE_TIME_CDC_SetSecondsDigit1(CFE_TIME_CDC_Clock_t *clock, int s);

/**
 * @brief Return the tens digit of the current seconds field.
 *
 * @param clock  Pointer to clock struct.
 * @return Current secondsDigit1 value.
 */
int CFE_TIME_CDC_GetSecondsDigit1(const CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Set the units digit of seconds, clamping to [0, CFE_TIME_CDC_SECONDARY_RADIX_MAX].
 *
 * @param clock  Pointer to clock struct.
 * @param s      New value for secondsDigit2.
 */
void CFE_TIME_CDC_SetSecondsDigit2(CFE_TIME_CDC_Clock_t *clock, int s);

/**
 * @brief Return the units digit of the current seconds field.
 *
 * @param clock  Pointer to clock struct.
 * @return Current secondsDigit2 value.
 */
int CFE_TIME_CDC_GetSecondsDigit2(const CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Reconfigure the clock's body maximums and normalize internal state.
 *
 * Applies the same normalization algorithm as the constructor.  Existing
 * time fields are preserved but may be inconsistent until reset.
 *
 * @param clock  Pointer to clock struct.
 * @param h      New maximum hours for the celestial body's day.
 * @param m      New maximum minutes for the celestial body's day.
 */
void CFE_TIME_CDC_SetBodyMaximums(CFE_TIME_CDC_Clock_t *clock, int h, int m);

/**
 * @brief Reconstruct the user-visible body maximums from normalized state.
 *
 * Inverts the normalization performed by CFE_TIME_CDC_SetBodyMaximums.
 *
 * @param clock        Pointer to clock struct.
 * @param bodyMaximums Output array of exactly 2 ints: [bodyMaxHours, bodyMaxMinutes].
 */
void CFE_TIME_CDC_GetBodyMaximums(const CFE_TIME_CDC_Clock_t *clock, int bodyMaximums[2]);

/**
 * @brief Format the current time as a military (24-hour) string.
 *
 * Output format: "H:MM:SS" where H is unpadded hours and MM, SS are the
 * concatenated digit pairs, e.g. "14:30:59".  Caller must supply a buffer
 * of at least CFE_TIME_CDC_TIME_STR_LEN bytes.
 *
 * @param clock   Pointer to clock struct.
 * @param buf     Caller-supplied output buffer.
 * @param bufLen  Size of buf in bytes.
 */
void CFE_TIME_CDC_GetTimeMilitary(const CFE_TIME_CDC_Clock_t *clock, char *buf, int bufLen);

/**
 * @brief Compute the AM/PM-equivalent hours value relative to the CDC half-day.
 *
 * Returns the hours field mapped to the first half of the planet day, which
 * is used to build the standard (AM/PM) time string.  The returned value may
 * exceed 12 for bodies with day lengths longer than 24 Earth hours.
 *
 * @param clock  Pointer to clock struct.
 * @return Hours value relative to the current half-day.
 */
int CFE_TIME_CDC_GetStandardHours(const CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Write the meridiem indicator string (" AM" or " PM") into buf.
 *
 * @param clock   Pointer to clock struct.
 * @param buf     Caller-supplied output buffer (at least 4 bytes).
 * @param bufLen  Size of buf in bytes.
 */
void CFE_TIME_CDC_GetMeridiemIndicator(const CFE_TIME_CDC_Clock_t *clock, char *buf, int bufLen);

/**
 * @brief Format the current time as an AM/PM-formatted string.
 *
 * Output format: "H:MM:SS XM", e.g. "7:30:59 PM".  The hour field reflects
 * the planet's half-day and may exceed 12 for bodies with long day lengths.
 * Caller must supply a buffer of at least CFE_TIME_CDC_TIME_STR_LEN bytes.
 *
 * @param clock   Pointer to clock struct.
 * @param buf     Caller-supplied output buffer.
 * @param bufLen  Size of buf in bytes.
 */
void CFE_TIME_CDC_GetTime(const CFE_TIME_CDC_Clock_t *clock, char *buf, int bufLen);

/**
 * @brief Fill both the military and standard time string buffers.
 *
 * Equivalent to calling CFE_TIME_CDC_GetTimeMilitary and
 * CFE_TIME_CDC_GetTime in sequence.  Both buffers must be at least
 * CFE_TIME_CDC_TIME_STR_LEN bytes.
 *
 * @param clock    Pointer to clock struct.
 * @param military Caller-supplied buffer for the military time string.
 * @param milLen   Size of military buffer in bytes.
 * @param standard Caller-supplied buffer for the standard time string.
 * @param stdLen   Size of standard buffer in bytes.
 */
void CFE_TIME_CDC_GetTimes(const CFE_TIME_CDC_Clock_t *clock,
                           char *military, int milLen,
                           char *standard, int stdLen);

/**
 * @brief Check whether the clock has reached its wrap-around point and
 *        reset it to zero (or the post-meridiem start) if so.
 *
 * @param clock  Pointer to clock struct.
 * @return true if the clock was reset, false otherwise.
 */
bool CFE_TIME_CDC_CheckTimeReset(CFE_TIME_CDC_Clock_t *clock);

/**
 * @brief Advance the clock by one second.
 *
 * Carries from secondsDigit2 -> secondsDigit1 -> minutes -> hours,
 * and calls CFE_TIME_CDC_CheckTimeReset before incrementing.
 *
 * @param clock  Pointer to clock struct.
 */
void CFE_TIME_CDC_Tick(CFE_TIME_CDC_Clock_t *clock);

#endif /* CFE_TIME_CDC_H */
