#pragma once

#include <stdint.h>

// Per-station snooze.
// A snoozed station is skipped by *scheduled* program runs until the snooze expires.
// Manual station runs, run-once programs and "run program now" still water it, so a
// snooze can always be overridden by hand. Snoozing a station that is running or queued
// stops it and drops its queued runs. Snooze times are kept in UTC and saved to a file,
// so they survive reboots and expire on their own.

constexpr uint32_t SNOOZE_MAX_MINUTES = 30UL * 24UL * 60UL;  // 30 days

void snooze_load();                               // call once at startup, after pd.init()
bool station_snoozed(uint8_t sid);                // true while sid's snooze is active
uint32_t station_snooze_remaining(uint8_t sid);   // seconds left, 0 if not snoozed
uint32_t station_snooze_until(uint8_t sid);       // UTC epoch when it ends, 0 if not snoozed
bool snooze_set(uint8_t sid, uint32_t minutes);   // minutes = 0 clears; false if not saved
