#include "stdafx.h"
#include "command_file_parser.h"
#include "REMUS_controller_t.h"	//KEEPALIVE_TIMER_MS
#include <cmath>	//floor

using namespace std;

ostream & operator<<(ostream & os, const override_t& this_override)
{
	os << "Aperture cooldown time: " << this_override.cooldown << " milliseconds" << endl;
	for(unsigned i=0; i < this_override.legs.size(); ++i) {
		os << "Leg " << i << endl;
		os << "\t" << this_override.legs[i].time << " milliseconds" << endl;
		os << "\t" << "Alter heading by " << this_override.legs[i].alter_heading << endl;
		os << "\t" << this_override.legs[i].depth_cmd << endl;
		os << "\t" << this_override.legs[i].speed_cmd << endl;
	}
	os << endl;
	return os;
}

int convert_seconds_string_to_int_milliseconds(string s)
{
	float tmp_float = 0;
	sscanf_s(s.c_str(), "%f", &tmp_float);
	return (int)(tmp_float*1000);
}

int read_cmds_file(const string filename, override_t & this_override, logger_t * const p_logger)
{
	ifstream cmds_in(filename);
	string line;
	vector<string> raw_cmds;

	if(!cmds_in) {
		ostringstream oss;
		oss << "Error reading command file: " << filename;
		print_and_log(p_logger, oss, LOG_ERROR);
		return -1;
	}

	while (getline(cmds_in, line)) {
		trim(line);
		
		//ignore blank lines and comments
		if(line.length() == 0 || line[0] == LINE_COMMENT_CHAR)
			continue;
		raw_cmds.push_back(line);
	}

	{
		ostringstream oss;
		oss << "Read " << raw_cmds.size() << " lines from command file: " << filename;
		print_and_log(p_logger, oss, LOG_INFO);
	}
	
	if(raw_cmds.size() % 4 != 2) {
		ostringstream oss;
		oss << "Sanity check on command file: " << filename << " failed - incorrect number of lines.";
		print_and_log(p_logger, oss, LOG_ERROR);
		return -2;
	}

	//get the cooldown time for this aperture
	this_override.cooldown = convert_seconds_string_to_int_milliseconds(raw_cmds[0]);
	if(this_override.cooldown == 0) {
		ostringstream oss;
		oss << "Cooldown in" << filename << " is 0 seconds" << endl;
		print_and_log(p_logger, oss, LOG_WARNING);
	}

	//Commands come in blocks of four (a "leg"), with the time in seconds first, then an alter heading command,
	// then a depth command (the keepalive), then a speed command
	for(unsigned i=1; i < raw_cmds.size()-1; i+=4) {
		override_leg_t leg;

		leg.time = convert_seconds_string_to_int_milliseconds(raw_cmds[i]);
		
		if(raw_cmds[i+1].find(">ALTER HEADING") == 0)
			sscanf_s(raw_cmds[i+1].substr(15).c_str(), "%f", &leg.alter_heading);
		else {
			ostringstream oss;
			oss << "ALTER HEADING command failed to parse in " << filename << ": command " << i+1;
			print_and_log(p_logger, oss, LOG_ERROR);
			return -2;
		}

		if(raw_cmds[i+2].find("#C,Depth") == 0) {
			leg.depth_cmd = raw_cmds[i+2];
		} else {
			ostringstream oss;
			oss << "Depth command failed to parse in " << filename << ": command " << i+2;
			print_and_log(p_logger, oss, LOG_ERROR);
			return -2;
		}

		if(raw_cmds[i+3].find("#C,Speed") == 0) {
			leg.speed_cmd = raw_cmds[i+3];
		} else {
			ostringstream oss;
			oss << "Speed command failed to parse in " << filename << ": command " << i+3;
			print_and_log(p_logger, oss, LOG_ERROR);
			return -2;
		}

		this_override.legs.push_back(leg);
	}

	if(raw_cmds[raw_cmds.size()-1].find(">RELEASE TO NEAREST")) {
		this_override.return_to_start = false;
	} else if(raw_cmds[raw_cmds.size()-1].find(">RELEASE TO START")) {
		this_override.return_to_start = true;
	} else {
		ostringstream oss;
		oss << "Failed to parse >RELEASE command in " << filename;
		print_and_log(p_logger, oss, LOG_ERROR);
		return -2;
	}

	cmds_in.close();
	return 0;
}

int command_file_parser_t::init_command_table(vector<int> & tags_to_ignore, map<int,override_t> & override_table, int & global_cooldown)
{
	{
		ostringstream oss;
		oss << "Intializing Command Table";
		print_and_log(p_logger, oss,LOG_INFO);
	}

	//read tags->aperatures table
	ifstream tags_in(TAG_TABLE_FILE);
	if(!tags_in) {
		ostringstream oss;
		oss << "Unable to open tag table file";
		print_and_log(p_logger, oss,LOG_FATAL);
		return -1;
	}

	string line;
	int cmd_file_count = 0;

	//hackish, but global cooldown should be first line of the file
	bool do_read_global_cooldown = true;

	while(getline(tags_in, line)) {
		int id; string filename;

		//ignore blank lines and comments
		if(line.length() == 0 || line[0] == LINE_COMMENT_CHAR)
			continue;

		istringstream is(line);

		//read the ID
		if (!(is >> id)) {
			ostringstream oss;
			oss << "Failed to parse line in tag table file:" << endl << "\t" << line;
			print_and_log(p_logger,oss,LOG_FATAL);
			return -2;
		}

		//This is hackish, but it avoids duplicating code. The global cooldown time should
		// probably live in another parameters file...
		if(do_read_global_cooldown) {
			global_cooldown = id;
			
			{
				ostringstream oss;
				oss << endl << "Global aperture cooldown time: " << global_cooldown << " seconds" << endl;
				print_and_log(p_logger, oss, LOG_INFO);
			}

			do_read_global_cooldown = false;
			continue;
		}
		
		{
			ostringstream oss;
			oss << "Reading command file for ID: " << id;
			print_and_log(p_logger, oss,LOG_INFO);
		}

		//read the filename and remove leading/trailing whitespace
		getline(is, filename);
		trim(filename);

		override_t this_override;

		if(filename == "IGNORE") {
			tags_to_ignore.push_back(id);
			{
				ostringstream oss;
				oss << "\t" << "Tag will be ignored." << endl;
				oss << endl;
				print_and_log(p_logger, oss, LOG_INFO);
			}

		} else if(!read_cmds_file(filename, this_override, p_logger) && this_override.legs.size() > 0) {
			override_table[id] = this_override;
			{
				ostringstream oss;
				oss << this_override << endl;
				print_and_log(p_logger, oss, LOG_INFO);
			}

			cmd_file_count++;
		} else if(this_override.legs.size() == 0) {
			ostringstream oss;
			oss << "Command file for ID: " << id << " (" << filename << ") contained no commands and was ignored.";
			print_and_log(p_logger, oss,LOG_ERROR);
		}
	}

	{
		ostringstream oss;
		oss << "Read " << cmd_file_count << " command files into memory." << endl;
		print_and_log(p_logger, oss,LOG_INFO);
	}

	if(cmd_file_count == 0) {
		ostringstream oss;
		oss << "No command files found. Aborting." << endl;
		print_and_log(p_logger, oss,LOG_FATAL);
		return -1;
	}

	tags_in.close();
	return 0;
}
