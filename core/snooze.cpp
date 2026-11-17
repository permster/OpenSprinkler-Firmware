#include "snooze.h"

#include "../OpenSprinkler.h"
#include "program.h"
#include "scheduler.h"
#include "../storage/files.h"

#include <cstring>

extern OpenSprinkler os;
extern ProgramData pd;

#if defined(ARDUINO)
#define SNOOZE_FILENAME "/snooze.dat"
#else
#define SNOOZE_FILENAME "snooze.dat"
#endif

namespace {

uint32_t snooze_until[MAX_NUM_STATIONS];  // UTC epoch seconds, 0 = not snoozed

// now_tz() is controller-local time; stored times are UTC so a timezone change
// doesn't move the end of a snooze.
uint32_t utc_now() {
	return (uint32_t)(os.now_tz() - (int32_t)3600 / 4 * (int32_t)(os.iopts[IOPT_TIMEZONE] - 48));
}

// Earliest still-active queue entry for sid, or 0xFF (same logic as the private
// helper used by /cm in api/commands.cpp).
uint8_t station_queue_id(uint8_t sid, uint32_t curr_time) {
	uint8_t queue_id = 0xFF;
	for (uint8_t candidate = 0; candidate < pd.nqueue; candidate++) {
		const RuntimeQueueStruct& entry = pd.queue[candidate];
		if (entry.sid != sid || !entry.dur || curr_time >= entry.deque_time) continue;
		if (queue_id == 0xFF || entry.st < pd.queue[queue_id].st) queue_id = candidate;
	}
	return queue_id;
}

void stop_station_runs(uint8_t sid) {
	uint32_t curr_time = os.now_tz();
	// same steps as turning a station off via /cm?en=0, repeated for every queued run
	for (uint16_t guard = 0; guard < RUNTIME_QUEUE_SIZE; guard++) {
		uint8_t qid = station_queue_id(sid, curr_time);
		if (qid == 0xFF) break;
		pd.station_qid[sid] = qid;
		pd.queue[qid].deque_time = curr_time;
		turn_off_station(sid, curr_time, 1);  // 1 = shift later sequential runs up
	}
}

}  // namespace

void snooze_load() {
	if (!file_read_block(SNOOZE_FILENAME, snooze_until, 0, sizeof(snooze_until))) {
		memset(snooze_until, 0, sizeof(snooze_until));
	}
}

uint32_t station_snooze_until(uint8_t sid) {
	if (sid >= MAX_NUM_STATIONS) return 0;
	uint32_t until = snooze_until[sid];
	return (until && utc_now() < until) ? until : 0;
}

bool station_snoozed(uint8_t sid) {
	return station_snooze_until(sid) != 0;
}

uint32_t station_snooze_remaining(uint8_t sid) {
	uint32_t until = station_snooze_until(sid);
	return until ? until - utc_now() : 0;
}

bool snooze_set(uint8_t sid, uint32_t minutes) {
	if (sid >= MAX_NUM_STATIONS) return false;
	if (minutes > SNOOZE_MAX_MINUTES) minutes = SNOOZE_MAX_MINUTES;
	snooze_until[sid] = minutes ? utc_now() + minutes * 60UL : 0;
	bool saved = file_write_block(SNOOZE_FILENAME, snooze_until, 0, sizeof(snooze_until));
	if (minutes) stop_station_runs(sid);
	return saved;
}
