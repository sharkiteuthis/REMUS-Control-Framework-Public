#ifndef LOGGER_H
#define LOGGER_H

#include "stdafx.h"

#define LOG_INFO	1
#define LOG_WARNING 2
#define LOG_ERROR	3
#define LOG_FATAL	4

bool open_log(ofstream & log_out);
int write_log_entry(ostringstream & ss, short level=LOG_INFO);

#endif