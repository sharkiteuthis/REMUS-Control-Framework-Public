#include "stdafx.h"
#include "tag_listener_t.h"

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

int read_cmds_file(string filename, vector<string> & cmd_list, logger_t * p_logger)
{
	//TODO: check if string is escaped
	ifstream cmds_in(filename);
	string line;

	if(!cmds_in) {
		ostringstream oss;
		oss << "Error reading command file: " << filename;
		print_and_log(p_logger, oss, LOG_ERROR);
		return -1;
	}

	while (getline(cmds_in, line)) {
		trim(line);
		cmd_list.push_back(line);
	}

	{
		ostringstream oss;
		oss << "Read " << cmd_list.size() << " lines from command file: " << filename;
		print_and_log(p_logger, oss, LOG_INFO);
	}

	cmds_in.close();
	return 0;
}

int tag_listener_t::init_command_table()
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
	int count = 0;

	while(getline(tags_in, line)) {
		int id; string filename;

		if (line.empty())
			continue;

		istringstream is(line);
		
		//read the ID
		if (!(is >> id)) {
			ostringstream oss;
			oss << "Failed to parse line in tag table file:" << endl << "\t" << line;
			print_and_log(p_logger,oss,LOG_FATAL);
			return -2;
		}
		
		{
			ostringstream oss;
			oss << "Reading command file for ID: " << id;
			print_and_log(p_logger, oss,LOG_INFO);
		}

		//read the filename and remove leading/trailing whitespace
		getline(is, filename);
		trim(filename);

		vector<string> cmds;
		if(!read_cmds_file(filename, cmds, p_logger) && cmds.size() > 0) {
			cmds_table[id] = cmds;
		} else if(cmds.size() == 0) {
			ostringstream oss;
			oss << "Command file for ID: " << id << " contained no commands and was ignored.";
			print_and_log(p_logger, oss,LOG_ERROR);
		}

		count++;
	}

	{
		ostringstream oss;
		oss << "Read " << count << " command files into memory.";
		print_and_log(p_logger, oss,LOG_INFO);
	}

	tags_in.close();
	return 0;
}

void add_tag(string line)
{

}
