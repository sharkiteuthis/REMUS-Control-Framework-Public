#include "stdafx.h"
#include "tag_handler_t.h"
#include "tag_file_listener_thread.h"
#include "REMUS_synthetic_aperture_override.h"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	//setup the cleanup handle
	HANDLE h_cleanup_event = CreateEvent(NULL, TRUE, /*manual reset object - will stay signalled until reset */
										FALSE, _T("CLEANUP_EVENT"));
	if(h_cleanup_event == NULL) {
		cout << "Failed to initialize cleanup event." << endl;
		system("pause");
		return -1;
	}

	//set up Ctrl+Break to cleanly terminate main loop (i.e. signal the cleanup handle)
	if(!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE)) {
		cout << "Failed to setup console signal handler." << endl;
		system("pause");
		return -10;	
	}

	//setup the logger
	logger_t logger;
	if(!logger.open_log(LOG_DIR)) {
		cout << "Failed to initialize logger." << endl;
		system("pause");
		return -1;
	}

	//setup the tag handler - this will parse all the command files
	tag_handler_t tag_handler(TAG_LISTEN_FILE, &logger, h_cleanup_event);

	if(tag_handler.setup()) {
		system("pause");
		return -2;	
	}

	//attempt to read the startup delay, if no file found or there is an error, use the
	// default value (60s)
	int startup_delay = STARTUP_DELAY_DEFAULT;
	if(read_startup_delay(startup_delay)) {
		startup_delay = STARTUP_DELAY_DEFAULT;
		ostringstream oss;
		oss << "Startup delay set to default (" << startup_delay << " seconds)." << endl;
		print_and_log(&logger, oss);
	} else {
		ostringstream oss;
		oss << "Startup delay set to: " << startup_delay << " seconds" << endl;
		print_and_log(&logger, oss);
	}

	//read in spl cutoff
	float spl_cutoff = SPL_CUTOFF_DEFAULT;
	if(read_spl_cutoff(spl_cutoff)) {
		spl_cutoff = SPL_CUTOFF_DEFAULT;
		ostringstream oss;
		oss << "SPL cutoff set to default (" << spl_cutoff << ")." << endl;
		print_and_log(&logger, oss);
	} else {
		ostringstream oss;
		oss << "SPL cutoff set to: " << spl_cutoff << endl;
		print_and_log(&logger, oss);
	}

	//read in exclusion zones
	vector<exclusion_zone_t> zones;
	int count = read_exclusion_zones(zones);
	if(count > 0) {
		ostringstream oss;
		oss << "Parsed " << count << " lines in exclusion zone file" << endl;
		for(vector<exclusion_zone_t>::iterator it = zones.begin(); it != zones.end(); ++it) {
			oss << "\t" << *it << endl;
		}
		print_and_log(&logger, oss);
	} else if(count == 0) {
		ostringstream oss;
		oss << "No valid zones parsed from exclusion zone file.";
		print_and_log(&logger, oss, LOG_WARNING);
	} else {
		ostringstream oss;
		oss << "Exclusion zones file not found.";
		print_and_log(&logger, oss, LOG_WARNING);
	}

	//setup the tag listener thread - NB: this is the last thing we should do before starting 
	// the main loop since it will wait for the tag csv file to show up
	HANDLE h_tag_file_listener_thread = setup_tag_file_listener_thread(logger, tag_handler, h_cleanup_event);
	if(h_tag_file_listener_thread == NULL) {
		system("pause");
		return -3;
	}

	int retVal = REMUS_main_control_loop(logger, tag_handler, h_tag_file_listener_thread, h_cleanup_event, startup_delay, spl_cutoff, zones);

	{
		ostringstream oss;
		oss << "REMUS SAOVR main control loop exitted with return value " << retVal;
		print_and_log(&logger, oss);
	}

	CloseHandle(h_cleanup_event);
	system("pause");
	return 0;
}
