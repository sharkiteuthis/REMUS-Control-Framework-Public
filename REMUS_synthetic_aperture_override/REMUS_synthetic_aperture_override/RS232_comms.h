#ifndef RECON_COMMS_H
#define RECON_COMMS_H

#include "stdafx.h"
#include <vector>
#include <queue>

using namespace std;

HANDLE open_com_port(string portname, HANDLE & com_mutex);
BOOL close_com_port(HANDLE com_handle, HANDLE & com_mutex);

#endif
