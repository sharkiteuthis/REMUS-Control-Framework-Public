#ifndef COMMAND_FILE_PARSER_H
#define COMMAND_FILE_PARSER_H

#include "stdafx.h"
#include "logger_t.h"

using namespace std;

#define TAG_TABLE_FILE  "REMUS_SAOVR_data\\tag_table.txt"
#define DEFAULT_TAG_ID	-1

struct override_leg_t {
	int time;	//in milliseconds
	float alter_heading;
	string depth_cmd;	//this will be used as the keep-alive
	string speed_cmd;
};

struct override_t {
	vector<override_leg_t> legs;
	int cooldown; //in ms
	bool return_to_start;	//return to point that override was initiated

	friend ostream & operator<<(ostream & os, const override_t& this_override);
};

class command_file_parser_t {
public:
	command_file_parser_t(logger_t * const p_logger) : p_logger(p_logger) {}
	int init_command_table(vector<int> & tags_to_ignore, map<int,override_t> & override_table, int & global_cooldown);

private:
	logger_t * p_logger;
};

//These don't really belong here, but they're here anyway...
// trim from start
inline string &ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

// trim from end
inline string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// trim from both ends
inline string &trim(string &s) {
	return ltrim(rtrim(s));
}


#endif
