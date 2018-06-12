#ifndef REMUS_SYNTHETIC_APERTURE_OVERRIDE_H
#define REMUS_SYNTHETIC_APERTURE_OVERRIDE_H

#include "stdafx.h"
#include "logger_t.h"
#include "tag_handler_t.h"

using namespace std;

int REMUS_main_control_loop(logger_t & logger, tag_handler_t & tag_handler, HANDLE h_tag_file_listener_thread, HANDLE h_cleanup_event,
							const int startup_delay, const float spl_cutoff, const vector<exclusion_zone_t> & zones);

BOOL CtrlHandler(DWORD signal);

#endif