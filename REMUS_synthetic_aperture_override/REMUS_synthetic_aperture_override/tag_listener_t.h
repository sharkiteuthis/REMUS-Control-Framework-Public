#ifndef TAG_LISTENER_H
#define TAG_LISTENER_H

#include "stdafx.h"
#include <queue>
#include "logger_t.h"


using namespace std;

#define TAG_TABLE_FILE  "REMUS_SAOVR_data\\tag_table.txt"

class tag_listener_t {
public:
	tag_listener_t(string file, logger_t * p_logger, HANDLE h_dir_change) :
	  tagfile_name(file), p_logger(p_logger), h_dir_change(h_dir_change) 
	  {}

	int init_command_table();

	void add_tag(string line);

	logger_t * p_logger;
	HANDLE h_dir_change;

private:
	string tagfile_name;
	long long tagfile_pos;
	
	//tag ID and power
	queue<pair<string,float> > tags_heard;

	//map of command list by tag ID
	map<int,vector<string> > cmds_table;
};

#endif