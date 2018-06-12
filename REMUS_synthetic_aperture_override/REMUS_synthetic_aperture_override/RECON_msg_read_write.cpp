#include "stdafx.h"
#include "RECON_msg_read_write.h"


RECON_msg_reader_t::RECON_msg_reader_t(HANDLE com_handle) : 
	com_handle(com_handle), buf_ndx(0)
{};
	
bool RECON_msg_reader_t::get_next_msg(string & msg, bool & msg_good) 
{
	if(msg_queue.empty()) {
		int retVal = 0;
		if(retVal = read_msgs_into_queue())
			return false;
	} else {
		msg = msg_queue.front().first;
		msg_good = msg_queue.front().second;
		msg_queue.pop();
	}

	return true;
}

//Returns error code of 0x100 if the port could not be opened or 0x101 is the receiver 
// detected an error. If data are not received, this function will hang because no timeouts were set.

//It's not important that we catch every message coming from the REMUS, since it spams status messages,
// so we'll fill up the buffer, then throw away anything we can't process.
int RECON_msg_reader_t::read_msgs_into_queue()
{
	int retVal = 0;
	DWORD dwCommModemStatus;

	SetCommMask(com_handle, EV_RXCHAR | EV_ERR); //receive character event
	WaitCommEvent(com_handle, &dwCommModemStatus, 0); //wait for character
	
	while(buf_ndx < COM_BUFFER_LEN) {
		DWORD xfered = 0;
		if (dwCommModemStatus & EV_RXCHAR) {
			ReadFile (com_handle, &buf[buf_ndx], COM_BUFFER_LEN - buf_ndx, &xfered, 0);
			buf_ndx += xfered;
		} else if (dwCommModemStatus & EV_ERR) {
			retVal = 0x101;
			break;
		}
	}
	
	if(!retVal)
		process_msg_buffer();

	return retVal;
}

void RECON_msg_reader_t::process_msg_buffer()
{
	BYTE * msg_start = buf;
	BYTE * buf_cur   = buf;
	BYTE * buf_end   = &buf[buf_ndx];

	//throw out any partial message we might have in the start of the buffer
	for(; buf_cur < buf_end; buf_cur++) {
		if(*buf_cur == '#')
			break;
	}

	for(; buf_cur < buf_end; buf_cur++) {
		if(*buf_cur == '\n') {
			int msg_len = buf_cur - msg_start + 1;
			bool is_good = is_checksum_good(msg_start, msg_len);
			string s((char*)msg_start,msg_len);

			msg_queue.push(make_pair(s,is_good));
				
			msg_start = buf_cur + 1;
		}
	}

	//the only way this can happen is if REMUS sends us a message larger than the size of
	// the internal buffer (default 1024)
	bool msg_processed = (msg_start != buf);
	assert(msg_processed);

	//if we have part of a message left over in the buffer... trash it.
	buf_ndx = 0;
}
	
bool RECON_msg_reader_t::is_checksum_good(BYTE * buf, DWORD buflen)
{
	//Find the ",*FF\n" sequence at the end of the message and validate the checksum
	int chksum;
	int sum;

	BYTE * cptr = buf + buflen - 5; //pointer to the comma
	if(*cptr != ',')
		cptr--;		//possible \r
		
	if(*cptr != ',')
		return false;
	if(*(cptr+1) != '*')
		return false;

	//sum all of the characters including the asterix
	sum = 0;
	do {
		sum += *buf;
	} while(buf++ != (cptr+1));
		
	if(1 != sscanf_s((char*)buf,"%02X",&chksum))
		return false;

	return (chksum == (sum & 0x0FF));
}

RECON_msg_writer_t::RECON_msg_writer_t(HANDLE com_handle) : 
	com_handle(com_handle) 
	{}

bool RECON_msg_writer_t::write_message(string msg)
{
	msg += ",*";

	DWORD num_bytes_written = 0;
		
	//compute and write out the checksum and terminating \n
	BYTE term_buf[16];
	DWORD chksum = compute_checksum(msg);
	sprintf_s((char*)term_buf, 16, "%02X\n", chksum);

	msg += string((char*)term_buf);

	BOOL retVal = WriteFile(com_handle,msg.c_str(),msg.length(),&num_bytes_written, NULL);
	if(!retVal || num_bytes_written != msg.length())
		return false;	
		
	return true;
}

DWORD RECON_msg_writer_t::compute_checksum(string msg)
{
	DWORD sum = 0;
	for(unsigned i = 0; i < msg.length(); ++i) {
		sum += (DWORD)msg[i];
	}

	return sum & 0x0FF;
}
