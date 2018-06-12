#ifndef RECON_MSG_READ_WRITE_H
#define RECON_MSG_READ_WRITE_H

#include "stdafx.h"
#include <vector>
#include <queue>

using namespace std;

#define COM_BUFFER_LEN	1024

class RECON_msg_reader_t {
public:
	RECON_msg_reader_t(HANDLE com_handle);
	bool get_next_msg(string & msg, bool & msg_good);

private:
	HANDLE com_handle;
	BYTE buf[COM_BUFFER_LEN];
	DWORD buf_ndx;
	queue<pair<string,bool> > msg_queue;

	//Returns error code of 0x100 if the port could not be opened or 0x101 is the receiver 
	// detected an error. If data are not received, this function will hang because no timeouts were set.
	int read_msgs_into_queue();
	void process_msg_buffer();
	bool is_checksum_good(BYTE * buf, DWORD buflen);
};


class RECON_msg_writer_t {
public:
	RECON_msg_writer_t(HANDLE com_handle);
	bool write_message(string msg);

private:
	HANDLE com_handle;

	DWORD compute_checksum(string msg);
};


#endif