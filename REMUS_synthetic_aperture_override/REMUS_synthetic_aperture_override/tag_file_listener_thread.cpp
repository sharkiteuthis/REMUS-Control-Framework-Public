#include "stdafx.h"
#include "logger_t.h"
#include "tag_handler_t.h"

using namespace std;

HANDLE setup_tag_file_listener_thread(const logger_t & logger, tag_handler_t &tag_handler, HANDLE h_cleanup_event)
{
	LPTSTR lpDir = _T(TAG_LISTEN_DIR);

	// Watch the directory for file size change 
	tag_handler.h_dir_change = FindFirstChangeNotification( 
		lpDir,                         // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_SIZE);
 
	if(tag_handler.h_dir_change == INVALID_HANDLE_VALUE) {
		cout << "ERROR: FindFirstChangeNotification failed: " << GetLastError() << endl;
		return NULL;
	}

	while(_access_s(TAG_LISTEN_FILE,04)) {
		cout << "Waiting to verify tag file location..." << endl;
		Sleep(5000);
	}

	if(!tag_handler.test_open_file()) {
		cout << "Failed to open tag file." << endl;
		return NULL;
	}

	//attempt to create the thread that will continually look for new tags in the tag file
	HANDLE h_tag_file_listener_thread = CreateThread(NULL, 0, tag_file_reader_thread, &tag_handler, 0, NULL);
	if(h_tag_file_listener_thread == NULL) {
		cout << "CreateThread failed: " << GetLastError() << endl;
		return NULL;
	}

	return h_tag_file_listener_thread;
}

//This function runs on it's own thread, continually monitoring the tag file
DWORD WINAPI tag_file_reader_thread(LPVOID lpParam)
{
	tag_handler_t* p_tag_handler = (tag_handler_t*)lpParam;

	{
		ostringstream oss;
		oss << "Waiting for file change in directory: " << TAG_LISTEN_DIR << endl;
		print_and_log(p_tag_handler->p_logger, oss, LOG_INFO);
	}

	while(42) {
		DWORD dwWaitStatus = WaitForSingleObject(p_tag_handler->h_cleanup_event,0);
		if(dwWaitStatus == WAIT_OBJECT_0 || dwWaitStatus == WAIT_ABANDONED) {
			ostringstream oss;
			oss << "Tag listener thread received terminate signal" << endl;
			print_and_log(p_tag_handler->p_logger, oss, LOG_INFO);
			break;
		}

		//wait for the file change, but wake up every so often to check for the terminate signal
		dwWaitStatus = WaitForSingleObject(p_tag_handler->h_dir_change, FILE_CHANGE_WAIT_TIME_MS);
		ostringstream oss;
 
		switch (dwWaitStatus) { 
			case WAIT_OBJECT_0:

				p_tag_handler->process_new_tags();

				if(FindNextChangeNotification(p_tag_handler->h_dir_change) == FALSE) {
					oss << "ERROR: FindNextChangeNotification failed: " << GetLastError();
					print_and_log(p_tag_handler->p_logger, oss, LOG_ERROR);
				}
				break;
		}
	}

	if(FindCloseChangeNotification(p_tag_handler->h_dir_change) == FALSE) {
		ostringstream oss;
		oss << "ERROR: FindCloseChangeNotification failed: " << GetLastError();
		print_and_log(p_tag_handler->p_logger, oss, LOG_ERROR);
	}
	return 0;
}
