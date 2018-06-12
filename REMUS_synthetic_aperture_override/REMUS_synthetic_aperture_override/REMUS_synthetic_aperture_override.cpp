#include "stdafx.h"
#include "REMUS_synthetic_aperture_override.h"
#include "RS232_comms.h"
#include "REMUS_controller_t.h"
#include "logger_t.h"
#include "tag_handler_t.h"

using namespace std;

HANDLE g_h_cleanup_event = NULL;
logger_t * g_p_logger = NULL;
tag_handler_t * g_p_tag_handler = NULL;

unsigned long g_keepalive_ticks = 0;
bool terminate_loop_rcvd = false;

BOOL CtrlHandler(DWORD signal)
{
	if(signal == CTRL_BREAK_EVENT) {
		terminate_loop_rcvd = true;
		return TRUE;
	}
	return FALSE;
}

//This function runs on a timer queue timer at a rate set in the REMUS_controller_t header file 
// RECON needs to get a keepalive message every so often (0.2 Hz minimum) in order to keep functioning
VOID CALLBACK RECON_keepalive_callback(PVOID lpParam, BOOLEAN TimeOrWaitFired)
{
	g_keepalive_ticks++;

	//decrement tag cooldowns
	g_p_tag_handler->decrement_cooldowns();

	if(lpParam != NULL) {
		REMUS_controller_t * p_controller = (REMUS_controller_t *)lpParam;
		p_controller->write_keepalive_msg_blocking();
	} else {
		ostringstream oss;
		oss << "NULL parameter in keepalive callback function!";
		print_and_log(g_p_logger, oss,LOG_ERROR);
	}
}

//This function is meant to run on it's own thread - it continuously monitors messages
// from the REMUS and updates the vehicle status accordingly
DWORD WINAPI RECON_status_reader_thread(LPVOID lpParam)
{
	REMUS_controller_t * p_controller = (REMUS_controller_t *)lpParam;
	string msg; bool msg_good;
	unsigned long status_count = 0;

	//sentinal read
	p_controller->get_next_msg_blocking(msg, msg_good);

	//infinite loop - check the message, set the state, get the next message (pending mutex)
	while(42) {
		if(msg_good && is_status_message(msg)) {
			unsigned short status;
			float heading;
			position_t pos;

			int retVal = parse_status_message(msg,status,heading,pos);
			p_controller->set_state(status,heading,pos);

			//REMUS spits out status messages at ~9Hz, so filter them a little
			if(status_count % 80 == 0) {
				cout.flush();
				cout << "Parse returned: " << retVal << endl;
				cout << "Status Message: " << status_count << endl;
				cout << get_vehicle_status_string(status) 
					 << "Heading: " << p_controller->get_current_heading() << " "
					 << "Position: " << p_controller->get_current_position() << endl;
				
				if(p_controller->is_inside_exclusion_zone()) {
					cout << "\tInside Exclusion Zone" << endl;
				}

				cout << msg << endl;
				cout.flush();
			}

			if (status_count % 400 == 0) {
				ostringstream oss;
				oss << "Parse returned: " << retVal << endl;
				oss << "Status Message: " << status_count << endl;
				oss << get_vehicle_status_string(status) 
					 << "Heading: " << p_controller->get_current_heading() << " "
					 << "Position: " << p_controller->get_current_position() << endl;
				
				if(p_controller->is_inside_exclusion_zone()) {
					oss << "\tInside Exclusion Zone" << endl;
				}
				
				oss << msg << endl;
				p_controller->p_logger->write(oss);		//log only, we already printed this message
			}

			status_count++;
		}

		DWORD dwWaitStatus = WaitForSingleObject(g_h_cleanup_event,0);
		if(dwWaitStatus == WAIT_OBJECT_0 || dwWaitStatus == WAIT_ABANDONED) {
			ostringstream oss;
			oss << "Status reader thread received terminate signal" << endl;
			print_and_log(g_p_logger, oss, LOG_INFO);
			return 0;
		}

		p_controller->get_next_msg_blocking(msg, msg_good);
	}
}

int REMUS_main_control_loop(logger_t &logger, tag_handler_t & tag_handler, HANDLE h_tag_file_listener_thread, HANDLE h_cleanup_event,
							const int startup_delay, const float spl_cutoff, const vector<exclusion_zone_t> & zones)
{
	int retval = 0;

	//setup file globals
	g_h_cleanup_event = h_cleanup_event;
	g_p_logger = &logger;
	g_p_tag_handler = &tag_handler;

	HANDLE com_mutex = NULL;
	HANDLE com_handle = open_com_port("COM1",com_mutex);

	//do this here to prevent spurious warnings with the goto
	REMUS_controller_t REMUS_controller(com_handle, com_mutex, tag_handler.h_tags_heard, startup_delay, spl_cutoff, zones, &logger);

	if(NULL == com_handle || NULL == com_mutex) {
		cout << "Failed to initialize COM port." << endl;
		retval = -1;
		goto exit_and_close_com_port;
	}

	if(!REMUS_controller.check_status_OK()) {
		cout << "Failed to initialize REMUS controller." << endl;
		retval = -1;
		goto exit_and_close_com_port;
	}

	tag_handler.p_controller = &REMUS_controller;

	//attempt to create the thread that will continually read messages from REMUS
	HANDLE hReaderThread = CreateThread(NULL,0,RECON_status_reader_thread, &REMUS_controller, 0, NULL);
	if(hReaderThread == NULL) {
		cout << "CreateThread failed: " << GetLastError() << endl;
		retval = -1;
		goto exit_and_close_com_port;
	}

	HANDLE hKeepaliveTimer = NULL;
	if(!CreateTimerQueueTimer(&hKeepaliveTimer,NULL, RECON_keepalive_callback, &REMUS_controller, KEEPALIVE_TIMER_MS, KEEPALIVE_TIMER_MS, 0)) {
		cout << "CreateTimerQueueTimer failed: " << GetLastError() << endl;
		retval = -1;
		goto exit_and_close_com_port;
	}

	{
		ostringstream oss;
		oss << endl << "SETUP COMPLETE: " << logger.get_current_time_str();
		print_and_log(&logger, oss);
	}

	//wait for the REMUS to signal that the mission has started
	if(!REMUS_controller.wait_for_mission_start_blocking()){
		retval = -1;
		goto exit_and_close_com_port;
	}
	
	{
		ostringstream oss;
		oss << endl << "MISSION START: " << g_p_logger->get_current_time_str();
		print_and_log(g_p_logger, oss);
	}

	//initial wait period, then clear the "tags heard" event so that we can start over fresh.
	Sleep(REMUS_controller.get_startup_delay_ms());
	ResetEvent(REMUS_controller.h_tags_heard);

	{
		ostringstream oss;
		oss << endl << "Startup delay ended: " << g_p_logger->get_current_time_str();
		print_and_log(g_p_logger, oss);
	}

	//MAIN LOOP
	while(!terminate_loop_rcvd) {
		//wait for a point when the mission is not suspended (stuck on surface, getting gps fix, etc)
		// this function also waits for a tag to be heard 
		if(!REMUS_controller.wait_multiple_for_tag_override()) {
			retval = -1;
			goto exit_and_close_com_port;
		}

		//we've heard a tag - get the commands for that tag and the tag id
		override_instance_t this_override = tag_handler.get_override_info();

		{
			ostringstream oss;
			oss << endl << "BEGIN OVERRIDE FOR TAG: " << this_override.first << " at " << logger.get_current_time_str() << endl;
			print_and_log(g_p_logger, oss);
			tag_handler.start_tag_cooldown(this_override);
		}

		//execute the override
		for(size_t i = 0; i < this_override.second.legs.size(); ++i) {
			//start the cooldown when we are finally about to start running the aperture
			ostringstream oss;
			oss << "Beginning wait for leg " << i << " at " << logger.get_current_time_str() << endl;
			//wait for a point when the mission is not suspended (stuck on surface, getting gps fix, etc)
			if(!REMUS_controller.wait_for_mission_un_suspended_blocking()) {
				retval = -1;
				goto exit_and_close_com_port;
			}

			oss << "Beginning leg " << i <<  " at " << logger.get_current_time_str() << endl;
			print_and_log(g_p_logger,oss);

			REMUS_controller.execute_override_leg(this_override.second, i);
		}
		
		{
			ostringstream oss;
			oss << endl;
			oss << "Beginning wait to release at " << logger.get_current_time_str() << endl;

			//wait for a point when the mission is not suspended (stuck on surface, getting gps fix, etc)
			if(!REMUS_controller.wait_for_mission_un_suspended_blocking()) {
				retval = -1;
				goto exit_and_close_com_port;
			}

			oss << "RELEASE OVERRIDE: " << logger.get_current_time_str() << endl;
			print_and_log(g_p_logger, oss);
		}

		REMUS_controller.release_override();
	}

	//signal the threads to terminate, then wait for them
	if(!SetEvent(g_h_cleanup_event)) {
		ostringstream oss;
		oss << "SetEvent (cleanup event) failed: " << GetLastError();
		print_and_log(g_p_logger,oss, LOG_ERROR);
		TerminateThread(hReaderThread,0);
		TerminateThread(h_tag_file_listener_thread,0);
	} else {
		ostringstream oss;
		oss << endl << "CLEANUP EVENT SENT TO THREADS" << endl;
		print_and_log(g_p_logger, oss);
		WaitForSingleObject(h_tag_file_listener_thread,INFINITE);
		WaitForSingleObject(hReaderThread,INFINITE);
	}

	//Wait for the COM port to kill the keepalive timer
	{
		DWORD retval = WaitForSingleObject(com_mutex, INFINITE);
		ostringstream oss;

		switch(retval) {			
			case WAIT_ABANDONED:
			case WAIT_OBJECT_0:
				if(!DeleteTimerQueueTimer(NULL, hKeepaliveTimer, INVALID_HANDLE_VALUE)) {
					cout<< "DeleteTimerQueueTimer failed : " <<  GetLastError() << endl;
				}

				if(!ReleaseMutex(com_mutex)) {
					oss << "Error releasing COM mutex in cleanup.";
					cout << oss.str() << endl;
					print_and_log(g_p_logger,oss, LOG_WARNING);
				}
				break;
			case WAIT_TIMEOUT:
				oss << "Timeout waiting for COM mutex in cleanup";
				print_and_log(g_p_logger,oss, LOG_WARNING);			
				break;
			default:
				oss << "Error waiting for COM mutex in cleanup: " << GetLastError();
				print_and_log(g_p_logger,oss, LOG_ERROR);
		}
	}

exit_and_close_com_port:
	close_com_port(com_handle, com_mutex);
	return retval;
}
