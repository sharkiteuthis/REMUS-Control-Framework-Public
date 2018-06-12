#include "stdafx.h"
#include "logger_t.h"

using namespace std;

bool logger_t::open_log(string dir)
{
	log_dir = dir;

	//convert char * to LPCWSTR to feed to CreateDirectory
	wstring wtmp = wstring(log_dir.begin(), log_dir.end());
	if(!CreateDirectory(wtmp.c_str(),NULL)) {
		DWORD err = GetLastError();
		if(err != ERROR_ALREADY_EXISTS) {
			cout << "Unable to create log directory: " << log_dir << endl;
			cout << "\t" << "Failed with error code: " << err << endl;
			return false;
		}
	}
	
	log_filename = log_dir + get_current_time_str() + ".txt"; 
	log_out.open(log_filename);

	if(!log_out) {
		cout << "Unable to open log file: " << log_filename << endl;
		return false;
	}

	ostringstream oss;
	oss << "Opened logfile at " << get_current_time_str() << endl;
	write(oss,LOG_INFO);

	cout << "Log file " << log_filename << " opened" << endl;
	cout.flush();
	return true;
}

string logger_t::get_current_time_str()
{
	time_t now = time(0);
	tm ltm;
	localtime_s(&ltm, &now);

	ostringstream log_ss;
	log_ss  << 1900 + ltm.tm_year << setw(2) << setfill('0') << 1+ltm.tm_mon 
			<< setw(2) << setfill('0') << ltm.tm_mday
			<< "_" << setw(2) << setfill('0') << 1+ltm.tm_hour << setw(2) << setfill('0')
			<< setw(2) << setfill('0') << ltm.tm_min << setw(2) << setfill('0') << ltm.tm_sec;
	return log_ss.str();
}

int logger_t::write(ostringstream & ss, short level) {
	if(!log_out) {
		cout << "Logger unable to write to logfile: " << log_filename << endl;
		return -2;
	}

	switch(level) {
		case LOG_WARNING:
			log_out << "WARNING: " << get_current_time_str() << endl;
			break;
		case LOG_ERROR:
			log_out << "ERROR: " << get_current_time_str() << endl;
			break;
		case LOG_FATAL:
			log_out << "FATAL ERROR: " << get_current_time_str() << endl;
			break;
	}

	log_out << ss.str() << endl;
	log_out.flush();

	if(!log_out)
		return -1;

	return 0;
}
