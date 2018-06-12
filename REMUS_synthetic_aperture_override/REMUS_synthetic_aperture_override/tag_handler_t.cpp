#include "stdafx.h"
#include "tag_handler_t.h"
#include "command_file_parser.h"
#include "REMUS_controller_t.h" //KEEPALIVE_TIMER_MS

extern logger_t * g_p_logger;

int milliseconds_to_ticks(int ms)
{
	return ms/KEEPALIVE_TIMER_MS;
}

int seconds_to_ticks(int seconds)
{
	return (seconds * 1000) / KEEPALIVE_TIMER_MS;
}

int parse_tag_line(string line, tag_instance_t& tag)
{
	trim(line);
		
	//second part of this if statement is a workaround for the way MapHost
	// writes out the header of it's CSV file (i.e with the first tag rather 
	// than when the file capture is started).
	if(line.length() == 0 || line[0] =='I')
		return -1;

	//lines look like this, roughly:
	//		ID,Data,Power,Time,dd/mm/yy
	//		15777,,23, 17:41:43.91625,11/10/15


	//now read in the tag ID
	//
	unsigned field_start = 0;
	unsigned field_end = line.find(",");
	if(field_end == string::npos) 
		return -2;

	string s = line.substr(field_start,field_end-field_start);
	sscanf_s(s.c_str(), "%i", &tag.id);

	//data field
	//
	field_start = field_end+1;
	field_end = line.find(",",field_start);
	if(field_end == string::npos) 
		return -3;
	//usually blank

	//spl
	//
	field_start = field_end+1;
	field_end = line.find(",",field_start);
	if(field_end == string::npos) 
		return -4;
	s = line.substr(field_start,field_end-field_start);
	sscanf_s(s.c_str(), "%f", &tag.spl);

	return 0;
}

tag_handler_t::tag_handler_t(string file, logger_t * p_logger, HANDLE h_cleanup_event) :
	tagfile_name(file), tagfile_pos(0), p_logger(p_logger), h_cleanup_event(h_cleanup_event)
{
	g_p_logger = p_logger;
}

int tag_handler_t::setup()
{
	command_file_parser_t parser(p_logger);

	vector<int> tags_to_ignore;
	if(parser.init_command_table(tags_to_ignore, override_table, global_cooldown))
	{
		ostringstream oss;
		oss << "Failed to read and initialize override command files." << endl;
		print_and_log(p_logger, oss, LOG_FATAL);
		return -1;
	}

	//we will ignore tags by putting them permanently in the cooldown table 
	// (i.e. put them in the table and don't remove or decrement them)
	vector<int>::const_iterator i = tags_to_ignore.begin();
	for( ; i != tags_to_ignore.end(); ++i)
		active_cooldowns[*i] = IGNORE_TAG;

	h_tags_heard = CreateEvent(NULL, FALSE, FALSE,_T("Tags Heard"));
	if(h_tags_heard == NULL) {
		ostringstream oss;
		oss << "CreateEvent failed (tags heard): " << GetLastError() << endl;
		print_and_log(p_logger, oss, LOG_FATAL);
		return -2;
	}

	if (!InitializeCriticalSectionAndSpinCount(&critical_section_tag_info, 0x00000400)) {
			ostringstream oss;
			oss << "Failed to init critical section: error " << GetLastError();
			print_and_log(p_logger, oss, LOG_FATAL);
			return -3;
	}

	return 0;
}

bool tag_handler_t::test_open_file()
{
	ifstream in(tagfile_name);
	if(!in) 
		return false;
	
	//set the start position of the tag file (generally zero)
	in.seekg(0, in.end);
	tagfile_pos = in.tellg();
	in.close();

	ostringstream oss;
	oss << "Successfully opened tag file: " << tagfile_name << endl
		<< "Start position: " << tagfile_pos << " bytes." << endl;
	print_and_log(p_logger, oss, LOG_INFO);


	return true;
}

bool tag_handler_t::is_cooldown_active(int tag_id)
{
	map<int,int>::iterator i = active_cooldowns.find(tag_id);
	if(i != active_cooldowns.end() && i->second > 0) {
		return true;
	}
	return false;
}

void tag_handler_t::process_new_tags()
{
	vector<tag_instance_t> new_tags;
	bool read_success = read_new_tag_instances_from_file(new_tags);

	//function should have written failure case to log
	if(!read_success)
		return;

	//check if we're inside an exclusion zone
	if(p_controller->is_inside_exclusion_zone())
		return;

	//check if the global cooldown is active
	if(is_cooldown_active(GLOBAL_COOLDOWN_ID))
		return;

	bool bTagSet = false;
	//find the last tag heard that has an inactive cooldown
	for(vector<tag_instance_t>::reverse_iterator ri = new_tags.rbegin(); ri != new_tags.rend(); ++ri) {
		if(is_cooldown_active(ri->id)) {
			ostringstream oss;
			oss << "Ignoring tag (cooldown active):" << *ri << endl;
			print_and_log(p_logger, oss, LOG_INFO);
			continue;
		}

		if(!p_controller->is_above_spl_cutoff(ri->spl)) {
			ostringstream oss;
			oss << "Ignoring tag (tag below SPL cutoff):" << *ri << endl;
			print_and_log(p_logger, oss, LOG_INFO);
			continue;		
		}

		if(bTagSet) {
			ostringstream oss;
			oss << "Ignoring tag (event already set):" << *ri << endl;
			print_and_log(p_logger, oss, LOG_INFO);
			continue;
		}

		// Must set this *before* signalling the event or else you get 
		//  a write-after-read condition
		EnterCriticalSection(&critical_section_tag_info);
		last_tag_heard = *ri;
		LeaveCriticalSection(&critical_section_tag_info);

		//set the tags heard event
		if(!SetEvent(h_tags_heard)) {
			ostringstream oss;
			oss << "Failed to set tags_heard event." << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		} else {
			ostringstream oss;
			oss << "Set tags_heard event for tag:" << *ri << endl;
			print_and_log(p_logger, oss, LOG_INFO);
			bTagSet = true;
		}
	}
}

override_instance_t tag_handler_t::get_override_info()
{
	EnterCriticalSection(&critical_section_tag_info);
	tag_instance_t local_tag_copy = last_tag_heard;
	LeaveCriticalSection(&critical_section_tag_info);
	
	map<int,override_t>::iterator i = override_table.find(local_tag_copy.id);
	
	if(i != override_table.end()) {
		return make_pair(local_tag_copy.id, i->second);
	}

	i = override_table.find(DEFAULT_TAG_ID);
	if(i == override_table.end()) {
		ostringstream oss;
		oss << "No command sequence for default tag ID found - using first command file as default." << endl;
		print_and_log(p_logger, oss,LOG_WARNING);
		return make_pair(local_tag_copy.id, override_table[0]);
	}
		
	return make_pair(local_tag_copy.id, override_table[DEFAULT_TAG_ID]);
}

bool tag_handler_t::read_new_tag_instances_from_file(vector<tag_instance_t> & new_tags)
{
	ifstream in(tagfile_name);
	if(!in) {
		ostringstream oss;
		oss << "Failed to open tag file." << endl;
		print_and_log(p_logger, oss, LOG_ERROR);
		return false;
	}
	
	in.seekg(tagfile_pos, in.beg);
	if(!in) {
		ostringstream oss;
		oss << "Failed to seek to previous position in tag file (byte " << tagfile_pos << ")." << endl;
		print_and_log(p_logger, oss, LOG_ERROR);
		return false;
	}

	string line;
	while (getline(in, line)) {
		tag_instance_t tag;
		int i = parse_tag_line(line,tag);

		if(i == 0) {
			new_tags.push_back(tag);
		} else if(i != -1) {			//remove spurious error for blank line at end of file
			ostringstream oss;
			oss << "Failed to parse tag line: (" << line << ")." << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		}

	}

	//we just read in all of the tags we heard, so increment the position in the file
	in.clear();		//need to clear EOF to get file position
	tagfile_pos = in.tellg();
	in.close();

	return true;
}

void tag_handler_t::start_tag_cooldown(const override_instance_t & this_override)
{
	active_cooldowns[this_override.first] = milliseconds_to_ticks(this_override.second.cooldown);
	active_cooldowns[GLOBAL_COOLDOWN_ID] = seconds_to_ticks(global_cooldown);
}

void tag_handler_t::decrement_cooldowns()
{
	for(map<int,int>::iterator i = active_cooldowns.begin(); i != active_cooldowns.end(); ++i) {
		if(i->second == 0 || i->second == IGNORE_TAG) {
			continue;
		}

		i->second--;

		if(i->second == 0) {
			ostringstream oss;

			if(i->first == GLOBAL_COOLDOWN_ID) {
				oss << "Global cooldown ended";
			} else {
				oss << "Cooldown ended for tag: " << i->first;
			}
			oss << " at " << g_p_logger->get_current_time_str() << endl;
			print_and_log(p_logger, oss, LOG_INFO);
		}
	}
}

ostream & operator<<(ostream & os, const tag_instance_t& t) {
	os << t.id << "   (spl: " << t.spl << ")";
	return os;
}
