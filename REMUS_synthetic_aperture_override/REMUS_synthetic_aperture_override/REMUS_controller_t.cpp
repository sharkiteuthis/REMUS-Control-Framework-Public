#include "stdafx.h"
#include "REMUS_controller_t.h"
#include <cmath>

using namespace std;

#define REMUS_STATE_PRELAUNCH(x)			((x & 0x0F) == 0x00)
#define REMUS_STATE_IN_MISSION(x)			(x & 0x01)
#define REMUS_STATE_MISSION_SUSPENDED(x)	(x & 0x02)
#define REMUS_STATE_MISSION_OVER(x)			(x & 0x03)
#define REMUS_STATE_OVERRIDE_ENABLED(x)		(x & 0x04)
#define REMUS_STATE_OVERRIDE_ACTIVE(x)		(x & 0x08)
#define REMUS_STATE_SHORE_POWER(x)			(x & 0x10)
#define REMUS_STATE_RETURN_CLOSEST(x)		(x & 0x20)
#define REMUS_STATE_DEPTH_OVERRIDE(x)		(x & 0x40)
#define REMUS_STATE_OVERRIDE_NOT_ALLOWED(x)	(x & 0x80)

const string default_keepalive_msg = "#V,Sep 14 2011, 13:17:09";

REMUS_controller_t::REMUS_controller_t(HANDLE com_handle, HANDLE com_mutex, HANDLE h_tags_heard, const int startup_delay, 
					const float spl_cutoff, const vector<exclusion_zone_t> & zones, logger_t * const p_logger) :
	com_mutex(com_mutex),
	reader(com_handle),
	writer(com_handle),
	p_logger(p_logger),
	keepalive_msg(default_keepalive_msg),
	h_tags_heard(h_tags_heard),
	spl_cutoff(spl_cutoff),
	startup_delay(startup_delay),
	exclusion_zones(zones),
	bCriticalSectionOK(false),
	bInExclusionZone(false)
{
	h_mission_started = CreateEvent(NULL, TRUE, FALSE,_T("Mission Started"));
	if(h_mission_started == NULL) {
		cout << "CreateEvent failed (mission start): " << GetLastError() << endl;
	}
	h_mission_un_suspended = CreateEvent(NULL, TRUE, FALSE,_T("Mission Not Suspended"));
	if(h_mission_un_suspended == NULL) {
		cout << "CreateEvent failed (mission not suspended): " << GetLastError() << endl;
	}
	h_override_not_enabled = CreateEvent(NULL, TRUE, FALSE,_T("Override Not Enabled"));
	if(h_override_not_enabled == NULL) {
		cout << "CreateEvent failed (override not enabled): " << GetLastError() << endl;
	}

	if (!InitializeCriticalSectionAndSpinCount(&critical_section_Keepalive_msg, 0x00000400)) {
		cout << "Failed to init critical section: error " << GetLastError();
	}
	bCriticalSectionOK = true;
}

REMUS_controller_t::~REMUS_controller_t()
{
	DeleteCriticalSection(&critical_section_Keepalive_msg);
	CloseHandle(h_mission_started);
	CloseHandle(h_mission_un_suspended);
	CloseHandle(h_override_not_enabled);
}

bool REMUS_controller_t::check_status_OK() {
	if(h_mission_started == NULL || h_mission_un_suspended == NULL || h_override_not_enabled == NULL
			|| !bCriticalSectionOK) {
		return false;
	}
	return true;
}

void REMUS_controller_t::set_state(const unsigned short & status, const float & heading, const position_t & pos)
{
	current_heading = heading;
	update_position(pos);

	if(REMUS_STATE_IN_MISSION(status)) {
		if(!SetEvent(h_mission_started)) {
			ostringstream oss;
			oss << "SetEvent (mission start) failed: " << GetLastError();
			print_and_log(p_logger, oss, LOG_ERROR);
		}
	} else {
		if(!ResetEvent(h_mission_started)) {
			ostringstream oss;
			oss << "ResetEvent (mission start) failed: " << GetLastError() << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		}
	}

	//despite the confusing name, the "override not allowed" is closer to a "suspended" state
	// that the state that we wish to indicate with the h_override_allowed handle (see docs)
	if(!REMUS_STATE_MISSION_SUSPENDED(status) && !REMUS_STATE_OVERRIDE_NOT_ALLOWED(status)) {
		if(!SetEvent(h_mission_un_suspended)) {
			ostringstream oss;
			oss << "SetEvent (mission not suspended) failed: " << GetLastError() << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		}
	} else {
		if(!ResetEvent(h_mission_un_suspended)) {
			ostringstream oss;
			oss << "ResetEvent (mission not suspended) failed: " << GetLastError() << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		}
	}

	if(!REMUS_STATE_OVERRIDE_ENABLED(status)) {
		if(!SetEvent(h_override_not_enabled)) {
			ostringstream oss;
			oss << "SetEvent (override not enabled) failed: " << GetLastError() << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		}
	} else {
		if(!ResetEvent(h_override_not_enabled)) {
			ostringstream oss;
			oss << "ResetEvent (override not enabled) failed: " << GetLastError() << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
		}
	}

}

void REMUS_controller_t::write_keepalive_msg_blocking()
{
	EnterCriticalSection(&critical_section_Keepalive_msg);
	string s(keepalive_msg);
	LeaveCriticalSection(&critical_section_Keepalive_msg);

	//wait on the mutex for the COM port, but give up after half the counter 
	// time to avoid the timer running concurrent copies of this function
	write_msg_blocking(s, KEEPALIVE_TIMER_MS/2);
}

void REMUS_controller_t::write_msg_blocking(string msg, DWORD timeout /* =INFINITE */)
{
	DWORD retval = WaitForSingleObject(com_mutex, timeout);
	ostringstream oss;
	
	switch(retval) {
		case WAIT_ABANDONED:
		case WAIT_OBJECT_0:
			writer.write_message(msg);
			
			if(!ReleaseMutex(com_mutex)) {
				oss << "Error releasing COM mutex in write_msg_blocking: " << msg << " " << GetLastError();
				print_and_log(p_logger, oss, LOG_WARNING);				
			}
			break;
		case WAIT_TIMEOUT:
			oss << "Timed out waiting for COM mutex in write_msg_blocking: " << msg;
			print_and_log(p_logger, oss, LOG_WARNING);
			break;
		default:
			oss << "Error releasing COM mutex in write_msg_blocking: " << msg << " " << GetLastError();
			print_and_log(p_logger, oss, LOG_ERROR);
	}
}

DWORD REMUS_controller_t::get_next_msg_blocking(string & msg, bool & msg_good)
{
	//wait until we get the mutex to use the COM port, then
	// do a read for a single message and release the mutex
	DWORD retval = WaitForSingleObject(com_mutex, INFINITE);
	ostringstream oss;

	switch(retval) {			
		case WAIT_ABANDONED:
		case WAIT_OBJECT_0:
			if(!reader.get_next_msg(msg,msg_good)) {
				oss << "Error reading messages from REMUS";
				print_and_log(p_logger, oss, LOG_ERROR);				
			}
			if(!ReleaseMutex(com_mutex)) {
				oss << "Error releasing COM mutex in status check callback.";
				print_and_log(p_logger, oss, LOG_WARNING);
			}
			break;
		case WAIT_TIMEOUT:
			oss << "Timeout waiting for COM mutex in status check callback.";
			print_and_log(p_logger, oss, LOG_WARNING);			
			break;
		default:
			oss << "Error waiting for COM mutex in status check callback: " << GetLastError();
			cout << oss.str() << endl;
			print_and_log(p_logger, oss, LOG_ERROR);
	}
	return retval;
}

bool REMUS_controller_t::wait_for_mission_start_blocking()
{
	DWORD retval = WaitForSingleObject(h_mission_started, INFINITE);
	ostringstream oss;

	switch(retval) {
		case WAIT_OBJECT_0:
			return true;
		default:
			oss << "Error waiting for mission start event: " << GetLastError();
			print_and_log(p_logger, oss, LOG_ERROR);
			return false;
	}
	return false;
}

bool REMUS_controller_t::wait_multiple_for_tag_override()
{
	HANDLE lpHandles[3];
	lpHandles[0] = h_mission_un_suspended;
	lpHandles[1] = h_override_not_enabled;
	lpHandles[2] = h_tags_heard;
	DWORD retval = WaitForMultipleObjects(3,lpHandles, TRUE, INFINITE);
	ostringstream oss;

	switch(retval) {
		case WAIT_OBJECT_0:
		case WAIT_OBJECT_0+1:
		case WAIT_OBJECT_0+2:
			return true;
		default:
			oss << "Error waiting for override allowed (multiple): " << GetLastError();
			print_and_log(p_logger, oss, LOG_ERROR);
			return false;

	}
}

bool REMUS_controller_t::wait_for_mission_un_suspended_blocking()
{
	DWORD retval = WaitForSingleObject(h_mission_un_suspended, INFINITE);
	ostringstream oss;

	switch(retval) {
		case WAIT_OBJECT_0:
			return true;
		default:
			oss << "Error waiting for h_mission_un_suspended: " << GetLastError();
			print_and_log(p_logger, oss, LOG_ERROR);
			return false;
	}
	return false;
}

bool REMUS_controller_t::wait_for_override_disabled_blocking()
{
	DWORD retval = WaitForSingleObject(h_override_not_enabled, INFINITE);
	ostringstream oss;

	switch(retval) {
		case WAIT_OBJECT_0:
			return true;
		default:
			oss << "Error waiting for h_override_not_enabled: " << GetLastError();
			print_and_log(p_logger, oss, LOG_ERROR);
			return false;
	}
	return false;
}

string REMUS_controller_t::get_heading_cmd(float alter_heading)
{
	//add the current heading, the delta, then add 360 just to make sure it's a positive number
	// before taking the mod 360
	float new_heading = fmod(get_current_heading() + alter_heading + (float)360.0, (float)360.0);

	//heading command looks like this: #C,Heading,Goal,270.0
	ostringstream oss;
	oss << "#C,Heading,Goal,";
	oss << setprecision(1) << fixed << new_heading;

	return oss.str();
}

void REMUS_controller_t::send_override_leg_commands(const override_leg_t & leg)
{
	//set up the new keepalive message
	set_keepalive_msg(leg.depth_cmd);
	
	//issue the override commands
	write_msg_blocking(leg.depth_cmd);
	Sleep(100);
	write_msg_blocking(leg.speed_cmd);
	Sleep(100);
	string heading_cmd = get_heading_cmd(leg.alter_heading);
	write_msg_blocking(heading_cmd);
	Sleep(100);
}

void REMUS_controller_t::initiate_override(const bool return_to_start)
{
	write_msg_blocking("#E,1");
	Sleep(100);
	if(!return_to_start) {
		write_msg_blocking("#c,1");
		Sleep(100);
	}
}

void REMUS_controller_t::execute_override_leg(const override_t & this_override, const int leg_index)
{
	const override_leg_t & leg = this_override.legs[leg_index];
	bool do_initiate_override = (leg_index==0);
	//for some reason, you have to send a dummy command on the first leg of the first 
	// override in order for the REMUS to accept them, so send a dummy query
	if(do_initiate_override) {
		write_msg_blocking("#Q,Depth");
		Sleep(100);
	}

	send_override_leg_commands(leg);

	//now that we've sent the override parameters, initiate the override if necessary
	if(do_initiate_override) {
		initiate_override(this_override.return_to_start);
	}

	//put the main thread to sleep until the end of this leg
	Sleep(leg.time);
}

void REMUS_controller_t::release_override()
{
	write_msg_blocking("#E,0");
	set_keepalive_msg(default_keepalive_msg);
}

void REMUS_controller_t::set_keepalive_msg(string msg)
{
	ostringstream oss;
		
	EnterCriticalSection(&critical_section_Keepalive_msg);
	keepalive_msg = msg;
	LeaveCriticalSection(&critical_section_Keepalive_msg);

	oss << "Keepalive message successfully changed to: " << keepalive_msg << endl;
	print_and_log(p_logger, oss,LOG_INFO);

}

//TODO move this into position_t and include a similar output format
float parse_latlon(string s) {
	//39N30.5553
	//0123456789
	float result = 0;
	istringstream in(s);
	
	int d;char c;float f;
	in >> d >> c >> f;

	result = (float)d + (f/60.0f);
	if(c == 'S' || c == 'W')
		result *= -1;
	
	return result;
}

int read_startup_delay(int & startup_delay)
{
	ifstream in(STARTUP_DELAY_FILE);
	if(!in)
		return -1;

	in >> startup_delay;
	return 0;
}

int read_spl_cutoff(float & spl_cutoff)
{
	ifstream in(SPL_CUTOFF_FILE);
	if(!in)
		return -1;

	in >> spl_cutoff;
	return 0;
}

int read_exclusion_zones(vector<exclusion_zone_t> & zones)
{
	ifstream in(EXCLUSION_ZONES_FILE);
	if(!in)
		return -1;

	string line;
	int count = 0;
	while(getline(in,line)) {
		line = trim(line);
		if(line.length() == 0 || line[0] == LINE_COMMENT_CHAR)
			continue;
		
		exclusion_zone_t zone;
		istringstream iss(line);
		string lat,lon;
		iss >> lat >> lon >> zone.radius;
		zone.center.lat = parse_latlon(lat);
		zone.center.lon = parse_latlon(lon);
		zones.push_back(zone);
		count++;
	}

	return count;
}

void REMUS_controller_t::update_position(const position_t & p)
{
	current_position = p;
	bool in_zone = false;
	bool prev_in_zone = bInExclusionZone;

	for(vector<exclusion_zone_t>::iterator it = exclusion_zones.begin(); it != exclusion_zones.end(); ++it) {
		if(it->zone_contains(current_position)) {
			in_zone = true;
			break;
		}
	}

	//atomic update
	bInExclusionZone = in_zone;

	if(!prev_in_zone && in_zone) {
		ostringstream oss;
		oss << endl << "Entered exclusion zone at " << p_logger->get_current_time_str() << endl;
		print_and_log(p_logger, oss);
	} else if(prev_in_zone && !in_zone) {
		ostringstream oss;
		oss << endl << "Leaving exclusion zone at " << p_logger->get_current_time_str() << endl;
		print_and_log(p_logger, oss);
	}
}

string get_vehicle_status_string(unsigned short status)
{
	ostringstream out_ss;
	out_ss << "VEHICLE STATE: 0x" << hex << status << endl;
	if(REMUS_STATE_PRELAUNCH(status))
		out_ss << "\tPrelaunch" << endl;
	if(REMUS_STATE_IN_MISSION(status))
		out_ss << "\tIn Mission" << endl;
	if(REMUS_STATE_MISSION_SUSPENDED(status))
		out_ss << "\tMission Suspended" << endl;
//	if(REMUS_STATE_MISSION_OVER(status))
//		out_ss << "\tMission Over" << endl;
	if(REMUS_STATE_OVERRIDE_ENABLED(status))
		out_ss << "\tOverride Enabled" << endl;
	if(REMUS_STATE_OVERRIDE_ACTIVE(status))
		out_ss << "\tOverride Active" << endl;
	if(REMUS_STATE_SHORE_POWER(status))
		out_ss << "\tOn Shore Power" << endl;
	if(REMUS_STATE_RETURN_CLOSEST(status))
		out_ss << "\tReturn to closest point on trackline" << endl;
	if(REMUS_STATE_DEPTH_OVERRIDE(status))
		out_ss << "\tDepth-only Override" << endl;
	if(REMUS_STATE_OVERRIDE_NOT_ALLOWED(status))
		out_ss << "\tOverride not allowed" << endl;

	return out_ss.str();
}

//TODO: this needs to be split out into a message parser class
int parse_status_message(string msg, unsigned short & status, float & current_heading, position_t & current_position)
{
	//#S,T15:08:37.672,LAT39N30.5553,LON74W19.4386,D-0.10,DG0.00,A0.00,P-0.3,R3.0,TR0,TRG0,V0.00,H0.4,HR0.0,HG0.0,M10,l0,*48\n
	unsigned status_ndx = msg.rfind("M");
	unsigned heading_ndx = msg.find("H");
	unsigned lat_ndx = msg.find("LAT");
	unsigned lon_ndx = msg.find("LON");

	//
	//status bits
	//
	if(status_ndx == string::npos)
		return -1;
	sscanf_s(msg.substr(status_ndx+1,2).c_str(), "%x", &status);

	//
	//current heading
	//
	// H132.0,
	// 0123456
	//  ^    ^
	//
	if(heading_ndx == string::npos)
		return -2;
	unsigned comma_pos = msg.find(",", heading_ndx);
	if(comma_pos == string::npos)
		return -2;
	sscanf_s(msg.substr(heading_ndx+1,comma_pos - heading_ndx - 1).c_str(), "%f", &current_heading);
	
	//
	//latitude and longitude
	//
	//LAT39N30.5553,LON...
	//01234567890123456
	if(lat_ndx == string::npos || lon_ndx == string::npos)
		return -3;
	comma_pos = msg.find(',',lon_ndx);
	if(comma_pos == string::npos)
		return -3;
	current_position.lat = parse_latlon(msg.substr(lat_ndx+3,lon_ndx - lat_ndx - 4));
	current_position.lon = parse_latlon(msg.substr(lon_ndx+3,comma_pos - lon_ndx - 3));
	
	return 0;
}

//uses the Haversine formula, possible we may need something more
// accurate, such as https://en.wikipedia.org/wiki/Vincenty%27s_formulae#Inverse_problem
// more info here: https://en.wikipedia.org/wiki/Geographical_distance#Ellipsoidal-surface_formulae
float position_t::distance(const position_t & p)
{
	float R = 6371009; // mean of Earth's radius in metres
	float toRads = 0.0174532925199f;
	float phi1 = lat * toRads;
	float phi2 = p.lat * toRads;
	float delta_phi = (p.lat-lat) * toRads;
	float delta_lambda = (p.lon-lon) * toRads;

	//haversine function
	float a = sin(delta_phi/2) * sin(delta_phi/2) + 
				cos(phi1) * cos(phi2) * sin(delta_lambda/2) * sin(delta_lambda/2);

	float c = 2 * atan2(sqrt(a), sqrt(1-a));

	return R*c;
}

ostream& operator<<(ostream &os, const position_t & p)
{
	streamsize ss = os.precision();
	os << fixed << setprecision(6) << "(" << p.lat << "," << p.lon << ")";
	os << setprecision(ss);
	os.unsetf(ios_base::fixed);
	return os;
}

ostream& operator<<(ostream &os, const exclusion_zone_t & p)
{
	os << "Center: " << p.center << "  Radius: " << p.radius <<" meters";
	return os;
}