#ifndef TAG_LISTENER_THREAD_H
#define TAG_LISTENER_THREAD_H

#include "stdafx.h"

using namespace std;

HANDLE setup_tag_file_listener_thread(const logger_t & logger, tag_handler_t & p_tag_handler, HANDLE h_cleanup_event);

#endif