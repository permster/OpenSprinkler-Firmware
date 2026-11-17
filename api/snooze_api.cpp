#include "handler_context.h"
#include "handlers.h"

#include "../core/snooze.h"

#include <cstdlib>

/**
 * Snooze a station (skip it in scheduled programs for a while)
 * Command: /sz?pw=xxx&sid=x&t=x
 *
 * sid: station index (starting from 0, same as /cm)
 * t:   snooze length in minutes (max 43200 = 30 days); 0 clears the snooze
 */
void server_change_snooze(OTF_PARAMS_DEF) {
	if (!process_password(OTF_PARAMS)) return;

	if (!findKeyVal(FKV_SOURCE, tmp_buffer, TMP_BUFFER_SIZE, PSTR("sid"), true)) handle_return(HTML_DATA_MISSING);
	int sid = atoi(tmp_buffer);
	if (sid < 0 || sid >= os.nstations) handle_return(HTML_DATA_OUTOFBOUND);
	if (os.is_master_station(sid)) handle_return(HTML_NOT_PERMITTED);

	if (!findKeyVal(FKV_SOURCE, tmp_buffer, TMP_BUFFER_SIZE, PSTR("t"), true)) handle_return(HTML_DATA_MISSING);
	char* end = nullptr;
	long minutes = strtol(tmp_buffer, &end, 10);
	if (end == tmp_buffer || *end || minutes < 0 || (uint32_t)minutes > SNOOZE_MAX_MINUTES) {
		handle_return(HTML_DATA_OUTOFBOUND);
	}

	if (!snooze_set(static_cast<uint8_t>(sid), static_cast<uint32_t>(minutes))) handle_return(HTML_INTERNAL_ERROR);
	handle_return(HTML_SUCCESS);
}

/**
 * Station snooze status
 * Command: /jz?pw=xxx
 *
 * Returns {"snooze":[seconds remaining per station],"until":[UTC epoch per station]}
 * (0 = not snoozed)
 */
void server_json_snooze(OTF_PARAMS_DEF) {
	if (!process_password(OTF_PARAMS)) return;
	begin_response(res);
	print_header(OTF_PARAMS);

	bfill.emit_p(PSTR("{\"snooze\":["));
	for (unsigned char sid = 0; sid < os.nstations; sid++) {
		bfill.emit_p(PSTR("$L"), station_snooze_remaining(sid));
		if (sid != os.nstations - 1) bfill.emit_p(PSTR(","));
	}
	bfill.emit_p(PSTR("],\"until\":["));
	for (unsigned char sid = 0; sid < os.nstations; sid++) {
		bfill.emit_p(PSTR("$L"), station_snooze_until(sid));
		if (sid != os.nstations - 1) bfill.emit_p(PSTR(","));
	}
	bfill.emit_p(PSTR("]}"));
	handle_return(HTML_OK);
}
