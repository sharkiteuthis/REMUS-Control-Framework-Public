#include "stdafx.h"
#include "RS232_comms.h"

HANDLE open_com_port(string portname, HANDLE & com_mutex)
{
	//convert char * to LPCWSTR to feed to CreateDirectory
	wstring wtmp = wstring(portname.begin(), portname.end());
	
	DCB dcb;
	HANDLE com_handle = CreateFile(wtmp.c_str(), GENERIC_READ | GENERIC_WRITE, 
									0, NULL, OPEN_EXISTING, 0, NULL);
	if (!GetCommState(com_handle,&dcb)) {
		CloseHandle(com_handle);
		return NULL;
	}
	dcb.BaudRate = CBR_57600;
	dcb.ByteSize = 8; //8 data bits
	dcb.Parity = NOPARITY; //no parity
	dcb.StopBits = ONESTOPBIT; //1 stop
	if (!SetCommState(com_handle,&dcb)) {
		CloseHandle(com_handle);
		return NULL;
	}

	com_mutex = CreateMutex(NULL,FALSE,_T("RS32_MUTEX"));
	if(com_mutex == NULL) {
		printf("RS232_MUTEX CreateMutex failed (%d)\n", GetLastError());
		CloseHandle(com_handle);
		return NULL;
	}
	return com_handle;
}

BOOL close_com_port(HANDLE com_handle, HANDLE & com_mutex)
{
	CloseHandle(com_handle);
	CloseHandle(com_mutex);
	return true;
}
