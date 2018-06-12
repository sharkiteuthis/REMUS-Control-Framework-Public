#ifndef LOGGER_H
#define LOGGER_H

#include "stdafx.h"

#define LOG_INFO	1
#define LOG_WARNING 2
#define LOG_ERROR	3
#define LOG_FATAL	4

#define LOG_DIR		"REMUS_SAOVR_logs\\"

using namespace std;

class logger_t {
public:
	~logger_t() {
		log_out.close();
	}

	bool open_log(string log_dir);

	string get_current_time_str();

	int write(ostringstream & ss, short level=LOG_INFO);
private:

	ofstream log_out;
	string log_dir;
	string log_filename;
};

inline void print_and_log(logger_t * const p_logger, ostringstream & oss, short level=LOG_INFO)
{
	p_logger->write(oss,level);

	cout << oss.str() << endl;
	cout.flush();
}

#endif
