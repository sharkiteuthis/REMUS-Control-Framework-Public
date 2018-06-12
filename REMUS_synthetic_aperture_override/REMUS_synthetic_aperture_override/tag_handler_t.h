#ifndef TAG_HANDLER_H
#define TAG_HANDLER_H

#include "stdafx.h"
#include "logger_t.h"
#include "command_file_parser.h"
#include "REMUS_controller_t.h"

using namespace std;

#define TAG_LISTEN_DIR	"REMUS_SAOVR_data\\tag_file\\"
#define TAG_LISTEN_FILE	"REMUS_SAOVR_data\\tag_file\\tag_file.csv"

//every so often, wake up and check if the thread needs to be terminated.
#define FILE_CHANGE_WAIT_TIME_MS	15000

DWORD WINAPI tag_file_reader_thread(LPVOID lpParam);

//this is the tag ID we will use in the tag map to denote the global cooldown time
#define GLOBAL_COOLDOWN_ID	-2

//this is the special number we'll use for tags that are being ignored entirely
#define IGNORE_TAG	99999999

class tag_instance_t {
public:
	tag_instance_t() : id(-666), spl(-666.0) {}
	tag_instance_t(const tag_instance_t & t) : id(t.id), spl(t.spl) {}

	int id;
	float spl;

	friend ostream & operator<<(ostream & os, const tag_instance_t& t);
};

typedef pair<int, override_t> override_instance_t;

class tag_handler_t {
public:
	tag_handler_t::tag_handler_t(string file, logger_t * p_logger, HANDLE h_cleanup_event);

	~tag_handler_t()
	{
		DeleteCriticalSection(&critical_section_tag_info);
		CloseHandle(h_tags_heard);
	}

	int setup();
	
	bool test_open_file();
	
	void process_new_tags();
	void start_tag_cooldown(const override_instance_t & this_override);
	override_instance_t get_override_info();
	void decrement_cooldowns();

	// class owns this handle
	HANDLE h_dir_change;
	HANDLE h_tags_heard;	//this is closed in the reader thread
	
	//class does not own these handles
	HANDLE h_cleanup_event;
	logger_t * p_logger;
	REMUS_controller_t * p_controller;

private:
	bool is_cooldown_active(int tag_id);
	bool read_new_tag_instances_from_file(vector<tag_instance_t> & new_tags);

	string tagfile_name;
	unsigned long long tagfile_pos;
	
	//override table, by tag ID
	map<int,override_t> override_table;

	// in seconds
	int global_cooldown;
	
	//variable for controlling thread-safe access to tags heard
	CRITICAL_SECTION critical_section_tag_info;
	tag_instance_t last_tag_heard;	//could be a queue, etc for more complex maneuvers

	//these are the cooldowns that are currently active
	map<int,int> active_cooldowns;
};

#endif
