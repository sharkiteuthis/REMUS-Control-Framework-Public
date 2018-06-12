#ifndef REMUS_controller_T
#define REMUS_controller_T

#include "stdafx.h"
#include "RECON_msg_read_write.h"
#include "logger_t.h"
#include "command_file_parser.h"



//REMUS needs to recieve messages at 0.2-9Hz in order to not fault and surface, 
// this callback function is run on a separate thread via a timer
#define KEEPALIVE_TIMER_MS			2000    // 2s resolution



#define EXCLUSION_ZONES_FILE	"REMUS_SAOVR_data\\exclusion_zones.txt"
#define STARTUP_DELAY_FILE		"REMUS_SAOVR_data\\startup_delay.txt"
#define SPL_CUTOFF_FILE			"REMUS_SAOVR_data\\spl_cutoff.txt"

#define STARTUP_DELAY_DEFAULT	60
#define SPL_CUTOFF_DEFAULT		30.0f

using namespace std;

class position_t {
public:
	position_t() : lat(-666), lon(-666) {};
	position_t(const position_t &p) : lat(p.lat), lon(p.lon) {};

	float lat;
	float lon;

	//returns distance in meters
	float distance(const position_t & p);
};

ostream& operator<<(ostream &os,  const position_t & pos);

//simple exclusion circles for now
class exclusion_zone_t {
public:
	position_t center;
	float radius;				//meters

	bool zone_contains(const position_t & p)
	{
		return (center.distance(p) <= radius);
	}
};

ostream& operator<<(ostream &os,  const exclusion_zone_t & pos);


int read_startup_delay(int & startup_delay);
int read_spl_cutoff(float & spl_cutoff);
int read_exclusion_zones(vector<exclusion_zone_t> & zones);


inline bool is_status_message(string msg)
{
	return msg[1] == 'S';
}

inline bool is_error_message(string msg)
{
	return msg[1] == 'E';
}

inline bool is_control_message(string msg)
{
	return msg[1] == 'C';
}

string get_vehicle_status_string(unsigned short status);
int parse_status_message(string msg, unsigned short & status, float & current_heading, position_t & current_position);

class REMUS_controller_t {
public:
	REMUS_controller_t(HANDLE com_handle, HANDLE com_mutex, HANDLE h_tags_heard, const int startup_delay, const float spl_cutoff,
						const vector<exclusion_zone_t> & zones, logger_t* const p_logger);
	REMUS_controller_t::~REMUS_controller_t();

	bool check_status_OK();

	void write_keepalive_msg_blocking();
	void write_msg_blocking(string msg, DWORD timeout=INFINITE);
	DWORD get_next_msg_blocking(string & msg, bool & msg_good);

	void set_state(const unsigned short & status, const float & heading, const position_t & pos);

	string get_heading_cmd(float alter_heading);

	void initiate_override(const bool return_to_start);
	void execute_override_leg(const override_t & this_override, const int leg_index);
	void send_override_leg_commands(const override_leg_t & leg);
	void release_override();

	void set_keepalive_msg(string msg);

	bool wait_for_mission_start_blocking();
	
	//this waits for 1) override not enabled 2) mission not suspended 3) tag heard
	bool wait_multiple_for_tag_override();

	bool wait_for_mission_un_suspended_blocking();
	bool wait_for_override_disabled_blocking();

	float get_current_heading()
	{
		return current_heading;
	}

	position_t get_current_position()
	{
		return current_position;
	}

	int get_startup_delay_ms()
	{
		return startup_delay * 1000;
	}

	bool is_inside_exclusion_zone() {
		return bInExclusionZone;
	}

	//owns these handles
	HANDLE h_mission_started;
	HANDLE h_mission_un_suspended;
	HANDLE h_override_not_enabled;

	//does not own this handle
	HANDLE h_tags_heard;

	logger_t * p_logger;

	bool is_above_spl_cutoff(const float spl) {
		return spl >= spl_cutoff;
	}

private:
	HANDLE com_mutex;
	RECON_msg_reader_t reader;
	RECON_msg_writer_t writer;
	
	float current_heading;
	position_t current_position;
	bool bInExclusionZone;

	void update_position(const position_t & p);

	//variable for controlling thread-safe access to tags heard
	CRITICAL_SECTION critical_section_Keepalive_msg;
	bool bCriticalSectionOK;

	string keepalive_msg;

	int startup_delay;		//in seconds
	
	float spl_cutoff;

	vector<exclusion_zone_t> exclusion_zones;
};

#endif