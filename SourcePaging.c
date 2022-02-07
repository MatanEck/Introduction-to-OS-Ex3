//  SourcePaging.c  //
/**********************************************

Authors:
Dvir Katz
Matan Eckhaus Moyal

Project : Ex3 - Paging Table

* *********************************************/
/// Description: This is the functions module for the project Paging Table. Contains the different functions that main module uses.

// Includes:
#include "SourcePaging.h"

/// === thread_main ===
/// Description: This is the main function that runs for every line if the input file, by a thread.
///				 It gets the relevant data and tries to map pages into frames in the paging table.
/// Parameters:
///		lpParam - a void pointer that refers to args struct (p_args). It contains 
///				  the relevant data from every line in the input file.
///	Returns: DWORD value - 0 if succeeded, 1 if failed
DWORD WINAPI thread_main(LPVOID lpParam)
{
	/* Check if lpParam is NULL */
	if (NULL == lpParam) {
		printf("Error: pointer 'lpParam' is NULL\n");
		return STATUS_FAILURE;
	}
	DWORD paging_status = STATUS_FAILURE;
	int num_of_frames, num_of_pages, page_index, page_to_replace;
	int lru_switch = LRU_INIT, ret_val = 0, min_time = 0, time_out_counter = 0;
	int flag = 1, done_mapping = 0, mutex_owner;
	DWORD dwWaitResult;
	multi_struct* p_args;
	p_args = (multi_struct*)lpParam;
	num_of_frames = p_args->argument[p_args->thread_index].num_of_frames;
	num_of_pages = p_args->argument[p_args->thread_index].num_of_pages;

	while (flag) {
		while (p_args->argument[p_args->thread_index].input.start_time != curr_time)  // Polling for current time
		{
			time_out_counter++;		// if curr_time didn't update, Force it - to avoid deadlock and infinite loop
			if (time_out_counter == INT_MAX)
				curr_time = p_args->argument[p_args->thread_index].input.start_time;
		}
		mutex_owner = 1;
		while (mutex_owner)
		{
			dwWaitResult = WaitForSingleObject(ghMutex, INFINITE); // Just one thread can update the table or read from it
			switch (dwWaitResult)
			{
			case(WAIT_OBJECT_0): // give ownership to one thread to update table
			{
				mutex_owner = 0; // just for the thread in control
				page_index = p_args->argument[p_args->thread_index].input.va / PAGE_SIZE;
				switch (p_args->argument[p_args->thread_index].pages[page_index].valid) // check if the page is mapped
				{
				case TRUE: { // page is mapped
					if (p_args->argument[p_args->thread_index].pages[page_index].end_of_use < p_args->argument[p_args->thread_index].input.start_time + p_args->argument[p_args->thread_index].input.time_of_use)
						p_args->argument[p_args->thread_index].pages[page_index].end_of_use = p_args->argument[p_args->thread_index].input.start_time + p_args->argument[p_args->thread_index].input.time_of_use; // updates the new time for end of use
					p_args->argument[p_args->thread_index].pages[page_index].LRU_count = LRU_INIT;
					paging_status = STATUS_SUCCESS;
					break;
				}

				case FALSE: { // page is not mapped to frame
					// 1. Trying to find free frame:
					for (int i = 0; i < num_of_frames; i++) {
						if (p_args->argument[p_args->thread_index].frames[i] == 0) {		// found free frame - so we can map it
							p_args->argument[p_args->thread_index].pages[page_index].frame_number = i;
							p_args->argument[p_args->thread_index].pages[page_index].valid = 1;
							p_args->argument[p_args->thread_index].pages[page_index].end_of_use = p_args->argument[p_args->thread_index].input.start_time + p_args->argument[p_args->thread_index].input.time_of_use;
							p_args->argument[p_args->thread_index].pages[page_index].LRU_count = LRU_INIT;
							p_args->argument[p_args->thread_index].frames[i] = 1;
							paging_status = STATUS_SUCCESS;
							ret_val = write_to_file(p_args->argument[p_args->thread_index].h_output_file, p_args->argument[p_args->thread_index].input.start_time, page_index, p_args->argument[p_args->thread_index].pages[page_index].frame_number, 'P');
							if (ret_val == STATUS_FAILURE) {
								printf("Error: Error while writing to output file\n");
								return STATUS_FAILURE;
							}
							done_mapping = 1; // we done mapping the current line
							break; // (get out of the for loop)
						}
					}
					if (done_mapping) { break; } // if we done mapping the current line - go to end (get out the switch cases)
					// 2.  We didn't find a free frame - Check for page timeout:
					for (int i = 0; i < num_of_pages; i++) {
						if (p_args->argument[p_args->thread_index].pages[i].end_of_use <= p_args->argument[p_args->thread_index].input.start_time) {  // found page timeout
							if (lru_switch < p_args->argument[p_args->thread_index].pages[i].LRU_count) { // Update LRU
								lru_switch = p_args->argument[p_args->thread_index].pages[i].LRU_count;	  // Max value of LRU
								page_to_replace = i;
							}
						}
					}
					if (lru_switch > LRU_INIT) { // with LRU we found the page to replace
						// Page out the old mapping
						p_args->argument[p_args->thread_index].pages[page_to_replace].valid = 0;
						p_args->argument[p_args->thread_index].pages[page_to_replace].LRU_count = -2;
						ret_val = write_to_file(p_args->argument[p_args->thread_index].h_output_file, p_args->argument[p_args->thread_index].input.start_time, page_to_replace, p_args->argument[p_args->thread_index].pages[page_to_replace].frame_number, 'E');
						if (ret_val == STATUS_FAILURE) {
							printf("Error: Error while writing to output file\n");
							return STATUS_FAILURE;
						}
						// New paging 
						p_args->argument[p_args->thread_index].pages[page_index].frame_number = p_args->argument[p_args->thread_index].pages[page_to_replace].frame_number;
						p_args->argument[p_args->thread_index].pages[page_index].valid = 1;
						p_args->argument[p_args->thread_index].pages[page_index].end_of_use = p_args->argument[p_args->thread_index].input.start_time + p_args->argument[p_args->thread_index].input.time_of_use;
						p_args->argument[p_args->thread_index].pages[page_index].LRU_count = LRU_INIT;
						paging_status = STATUS_SUCCESS;
						ret_val = write_to_file(p_args->argument[p_args->thread_index].h_output_file, p_args->argument[p_args->thread_index].input.start_time, page_index, p_args->argument[p_args->thread_index].pages[page_index].frame_number, 'P');
						if (ret_val == STATUS_FAILURE) {
							printf("Error: Error while writing to output file\n");
							return STATUS_FAILURE;
						}
					}
					// 3. we didn't find timeout for page use - the line has to wait:
					else {
						min_time = find_min_end_time(p_args->argument[p_args->thread_index].pages, num_of_pages); // find the next end of use time
						if (min_time == STATUS_FAILURE) {
							printf("Error: Error while calculating the minimum end time\n");
							return STATUS_FAILURE;
						}
						p_args->argument[p_args->thread_index].input.start_time = min_time;
						paging_status = PAGE_WAIT;
					}
					break;
				}
				default: {
					done_mapping = 0;
					break; }
				}
				// end: in the end - update LRU and release mutex:
				update_lru(p_args->argument[p_args->thread_index].pages, num_of_pages);
				ret_val = update_current_time(curr_time, p_args);
				if (ret_val == -1) { // no update for the current time
				}
				else if (ret_val == STATUS_FAILURE) {
					printf("Error: failed to update current time\n");
					return STATUS_FAILURE;
				}
				else { curr_time = update_current_time(curr_time, p_args); }

				if (!ReleaseMutex(ghMutex))
				{
					printf("Error: Cannot handle the mutex\n");
					return STATUS_FAILURE;
				}
				if (paging_status != PAGE_WAIT)
					return paging_status;
			}
			}
		}
	}
}


/// === read_file_2_input_args ===
/// Description: This function reads the input file and convrt it to array of the
///				 object line_input (each line with it's data).
/// Parameters:
///		source_file - LPSTR value. the input file name.
///		p_lines_count -  int pointer that refers to the amount of lines in the file.	
///	Returns: input_line pointer - returns the relevant data inserted in the input line struct. Null if failed.
extern input_line* read_file_2_input_args(LPSTR file_name, int* p_lines_count)
{
	if (NULL == p_lines_count || NULL == file_name) {
		printf("Error: argument pointer is NULL\n");
		return NULL;
	}
	int ret_val = 0;
	input_line* lines_args = NULL;
	HANDLE h_file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h_file == INVALID_HANDLE_VALUE) { 		// If failed to open file
		printf("Error: Failed to open plain file! The error code is %d\n", GetLastError());
		return NULL; //error
	}
	DWORD file_size, dw_bytes_read = 0;
	file_size = GetFileSize(h_file, NULL);
	char* h_array = (char*)malloc(file_size * sizeof(char) + 1);
	if (h_array == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = CloseHandle(h_file);
		if (FALSE == ret_val) {
			printf("Error when closing file handle. The error is: %d\n", GetLastError());
			return NULL; //error
		}
		return NULL; //error
	}
	BOOL read_status = ReadFile(h_file, (void*)h_array, file_size, &dw_bytes_read, NULL);
	if (read_status == 0) {		// If failed to read file
		printf("Error: Failed to Read file! The error code is %d\n", GetLastError());
		free(h_array);
		ret_val = CloseHandle(h_file);
		if (FALSE == ret_val) {
			printf("Error when closing file handle. The error is: %d\n", GetLastError());
			return NULL; //error
		}
		return NULL; //error
	}
	if (dw_bytes_read > 0)
		h_array[dw_bytes_read] = '\0';

	lines_args = split_text_file_2_lines_arr(h_array, p_lines_count);
	if (lines_args == NULL) {
		printf("Error: A function failed\n");
		free(h_array);
		ret_val = CloseHandle(h_file);
		if (FALSE == ret_val) {
			printf("Error when closing file handle. The error is: %d\n", GetLastError());
			return NULL; //error
		}
		return NULL; //error
	}
	ret_val = CloseHandle(h_file);
	if (FALSE == ret_val) {
		printf("Error when closing file handle. The error is: %d\n", GetLastError());
		free(h_array);
		free(lines_args);
		return NULL; //error
	}
	free(h_array);
	return lines_args;
}

/// === update_lru ===
/// Description: This function updates the LRU counter.
/// Parameters:
///		p_pages - pointer to the list of pages.
///		num_of_pages - int value. The number of pages. 
///	Returns: VOID
extern void update_lru(page* p_pages, int num_of_pages)
{
	if (NULL == p_pages) {
		printf("Error: argument pointer is NULL\n");
		return;
	}
	for (int i = 0; i < num_of_pages; i++) {
		if (p_pages[i].LRU_count > -2)	// if the current page is in use - update the counter
			p_pages[i].LRU_count = p_pages[i].LRU_count + 1;
	}
}

/// === find_min_end_time ===
/// Description: This function returns the next end of use time.
/// Parameters:
///		p_pages - pointer to the list of pages.
///		num_of_pages - int value. The number of pages. 
///	Returns: int value for the next end of use time.
extern int find_min_end_time(page* p_pages, int num_of_pages) {
	if (NULL == p_pages) {
		printf("Error: argument pointer is NULL\n");
		return STATUS_FAILURE;
	}
	int min = INT_MAX;
	for (int i = 0; i < num_of_pages; i++)
	{
		if (min > p_pages[i].end_of_use && p_pages[i].valid == 1)
			min = p_pages[i].end_of_use;
	}
	return min;
}

/// === find_max_end_time ===
/// Description: This function returns the maximum end of use time.
/// Parameters:
///		p_pages - pointer to the list of pages.
///		num_of_pages - int value. The number of pages. 
///	Returns: int value for the maximum end of use time.
extern int find_max_end_time(page* p_pages, int num_of_pages) {
	if (NULL == p_pages) {
		printf("Error: argument pointer is NULL\n");
		return STATUS_FAILURE;
	}
	int max = -1;
	for (int i = 0; i < num_of_pages; i++)
	{
		if (max < p_pages[i].end_of_use && p_pages[i].valid == 1)
			max = p_pages[i].end_of_use;
	}
	return max;
}

/// === split_text_file_2_lines_arr ===
/// Description: This function converts string source to int array.
/// Parameters:
///		source_file - char pointer to a source string to be converted.
///		p_lines_count -  int pointer that refers to the amount of lines in the file.	
///	Returns: input_line pointer - returns the relevant data inserted in the input line struct. Null if failed.
extern input_line* split_text_file_2_lines_arr(char* source_file, int* p_lines_count) {
	if (NULL == source_file || NULL == p_lines_count) {
		printf("Error: argument pointer is NULL\n");
		return NULL;
	}
	input_line* line_args = NULL;
	char* token;
	char* ptr;
	int top = -1;
	*p_lines_count = 0;
	char* temp = (char*)malloc(strlen(source_file) + 1);
	if (temp == NULL) {
		printf("Error: Failed to allocate memory\n");
		return NULL; //error
	}
	strcpy(temp, source_file);
	// Extract the first token
	token = strtok(temp, "\r\n");
	// loop through the string to extract all other tokens
	while (token != NULL) {
		(*p_lines_count)++;
		token = strtok(NULL, "\r\n");
	}
	line_args = (input_line*)malloc(sizeof(input_line) * (*p_lines_count));
	if (line_args == NULL) {
		printf("Error: Failed to allocate memory\n");
		free(temp);
		return NULL; //error
	}
	token = strtok(source_file, "\r\n");
	while (token != NULL) {
		top++;
		line_args[top].start_time = strtol(token, &ptr, 10);
		line_args[top].va = strtol(ptr, &ptr, 10);
		line_args[top].time_of_use = strtol(ptr, &ptr, 10);
		token = strtok(NULL, "\r\n");
	}
	free(temp);
	return line_args;
}

/// === write_to_file ===
/// Description: This function writes relevant activity to the output file.
///				 It gets a the data that needs to be written, convert it to the format needed and write it into the file.
/// Parameters:
///		h_output_file - handle to the output file
///		curr_time - int value. The time value of the activity to be logged.
///		page_index - int value. The index number for the relevant page.
///		frame_number - int value. The frame number.
///		status - char value. The status of the activity logged - "P" - for paging in page into frame
///																 "E" - for evicting a frame
///	Returns: int value - 0 if succeeded, 1 if failed
extern DWORD write_to_file(HANDLE h_output_file, int curr_time, int page_index, int frame_number, char status) {
	if (NULL == h_output_file) {
		printf("Error: argument pointer is NULL\n");
		return STATUS_FAILURE;
	}
	DWORD str_out_len = 0, dw_bytes_written = 0;
	char* str_to_write = NULL;
	str_out_len = snprintf(NULL, 0, "%d %d %d %c\n", curr_time, page_index, frame_number, status);
	str_to_write = (char*)malloc(sizeof(char) * str_out_len + 1);
	if (NULL == str_to_write) {
		printf("Error: pointer is NULL, malloc failed\n");
		return STATUS_FAILURE;
	}
	snprintf(str_to_write, str_out_len + 1, "%d %d %d %c\n", curr_time, page_index, frame_number, status);
	BOOL write_status = WriteFile(h_output_file, (void*)str_to_write, str_out_len, &dw_bytes_written, NULL);
	if (write_status == 0 && str_out_len != dw_bytes_written) {
		printf("Error: File write partially or failed. The error code is %d\n", GetLastError());
		free(str_to_write);
		return STATUS_FAILURE;
	}
	free(str_to_write);
	return STATUS_SUCCESS;
}


/// === evict_left_pages ===
/// Description: This function evicts the last left pages from the mapping table (and log it accordingly).
/// Parameters:
///		p_pages - pointer to the list of pages.
///		frame_arr -  pointer to int array of the frames (value 1 in the list means that the frame is in use)
///		num_of_pages - int value. The number of pages.
///		num_of_frames - int value. The number of frames.
///		h_output_file - handle to the output file.
///	Returns: int value - 0 if succeeded, 1 if failed
extern int evict_left_pages(page* p_pages, int* frame_arr, int num_of_pages, int num_of_frames, HANDLE h_output_file) {
	if (NULL == p_pages || NULL == frame_arr || NULL == h_output_file) {
		printf("Error: argument pointer is NULL\n");
		return STATUS_FAILURE;
	}
	int ret_val = 0, j = 0;
	int curr_time = 0, curr_frame = 0;
	curr_time = find_max_end_time(p_pages, num_of_pages);	// evicting when all the threads are done - find the next end of time
	if (curr_time == STATUS_FAILURE) {
		printf("Error: Error while calculating the max end time\n");
		return STATUS_FAILURE;
	}
	// find the frames that are mapped.
	for (int i = 0; i < num_of_frames; i++) {
		if (frame_arr[i]) { // if the current frame is mapped
			curr_frame = i;
			for (int j = 0; j < num_of_pages; j++) {
				if (p_pages[j].valid == 1 && p_pages[j].frame_number == curr_frame) {
					// Page out the old mapping
					p_pages[j].valid = 0;
					p_pages[j].LRU_count = -2;
					ret_val = write_to_file(h_output_file, curr_time, j, p_pages[j].frame_number, 'E');
					if (ret_val == STATUS_FAILURE) {
						printf("Error: Error while writing to output file\n");
						return STATUS_FAILURE;
					}
				}
			}
		}
	}
	return STATUS_SUCCESS;
}

/// === update_current_time ===
/// Description: This function updates the clock and set the cuurrent time according to the input file.
/// Parameters:
///		curr_time -  int value. current time value
///		p_args - pointer to the list of args.
///	Returns: int value - the current time if an update found -1 if not found. returns 1 if failed
extern int update_current_time(int curr_time, multi_struct* p_args) {
	if (NULL == p_args) {
		printf("Error: argument pointer is NULL\n");
		return STATUS_FAILURE;
	}
	int temp = INT_MAX;
	BOOL race_condition;
	race_condition = check_race_condition(curr_time, p_args); // Check for race condition
	if (!race_condition) {
		for (int i = 0; i < p_args->num_of_threads; i++) {
			if (abs(curr_time - temp) > abs(curr_time - p_args->argument[i].input.start_time) && (curr_time - temp != 0)) {
				if (curr_time < p_args->argument[i].input.start_time)
					temp = p_args->argument[i].input.start_time;
			}
		}
		if (temp < INT_MAX)
			return temp;
		else
			return -1;
	}
	else
	{
		return curr_time;
	}
}

/// === check_race_condition ===
/// Description: The function check for race condition
/// Parameters:
///		curr_time -  int value. current time value
///		p_args - pointer to the list of args.
/// Returns: Boolean value - True if race condition exist else False
extern BOOL check_race_condition(int curr_time, multi_struct* p_args) {
	if (NULL == p_args) {
		printf("Error: argument pointer is NULL\n");
		return TRUE; // error
	}
	int counter = 0;
	for (int i = 0; i < p_args->num_of_threads; i++) {
		if (curr_time == p_args->argument[i].input.start_time) {
			counter++;
		}
	}
	if (counter > 1)
		return TRUE;
	else
		return FALSE;
}

/// === free_mem_close_handles ===
/// Description: This function closes all the relevant handles and frees the memory allocations. 
///				 It gets NULL when a parameter is not relevant so it won't be affected
/// Parameters:
///		p_lines_count -  int pointer that refers to the amount of lines in the file.	
///		p_thread_handles - handle pointer to list of thread handles
///		h_output_file - handle to the output file.
///		ghMutex - handle to the general mutex
///		thread_pages - pointer to list of thread pages
///		frame_arr -  pointer to int array of the frames (value 1 in the list means that the frame is in use)
///		args_for_thread - args pointer to arguments list for the threads main
///		p_thread_ids - DWORD pointer to a list of ids for every thread
///		lines_args - input_line pointer to a list of input lines objects (contain the data from input)
///		multi_arg - multi_struct pointer to a list of multi_struct objects
///		output_path - char pointer to the output path
///	Returns: int value - 0 if succeeded, 1 if failed
extern int free_mem_close_handles(int* p_lines_count, HANDLE* p_thread_handles, HANDLE h_output_file, HANDLE ghMutex,
	page* thread_pages, int* frame_arr, args* args_for_thread, DWORD* p_thread_ids,
	input_line* lines_args, multi_struct* multi_arg, char* output_path) {
	int exit_code = 0, i = 0, ret_val = 0;
	int status = 0; // if 0 the function succeeded, if status<0 - fails

	if (NULL != p_thread_handles) {
		for (i = 0; i < *p_lines_count; i++) {
			/* Check the ret_val returned by Thread */
			ret_val = GetExitCodeThread(p_thread_handles[i], &exit_code);
			if (0 == ret_val) {
				printf("Error when getting thread exit code\n");
				status--;
			}
			ret_val = CloseHandle(p_thread_handles[i]);
			if (FALSE == ret_val) {
				printf("Error when closing thread: %d\n", GetLastError());
				status--;
			}
		}
		free(p_thread_handles);
	}
	if (NULL != h_output_file) {
		ret_val = CloseHandle(h_output_file);
		if (FALSE == ret_val) {
			printf("Error when closing file handle. The error is: %d\n", GetLastError());
			status--;
		}
	}
	if (NULL != ghMutex) {
		ret_val = CloseHandle(ghMutex);
		if (FALSE == ret_val) {
			printf("Error when closing file handle. The error is: %d\n", GetLastError());
			status--;
		}
	}
	if (NULL != thread_pages) { free(thread_pages); }
	if (NULL != frame_arr) { free(frame_arr); }
	if (NULL != args_for_thread) { free(args_for_thread); }
	if (NULL != p_thread_ids) { free(p_thread_ids); }
	if (NULL != lines_args) { free(lines_args); }
	if (NULL != multi_arg) { free(multi_arg); }
	if (NULL != output_path) { free(output_path); }

	if (status < 0)
		return STATUS_FAILURE;
	return STATUS_SUCCESS;
}

/// === set_output_path ===
/// Description: The function sets the output path according to the location ot the input path
/// Parameters:
///		p_input_path - char pointer to the input path
/// Returns: char pointer to the output path
extern char* set_output_path(char* p_input_path) {
	if (NULL == p_input_path) {
		printf("Error: argument pointer is NULL\n");
		return NULL; // error	
	}
	char* ret = NULL;
	int len = strlen(p_input_path) + 2;
	char* input_copy = (char*)malloc(len * sizeof(char));
	if (NULL == input_copy) {
		printf("Error: pointer is NULL, malloc failed\n");
		return NULL; // error	
	}
	strcpy(input_copy, p_input_path); // first creates a local copy of the input path

	char* output_path = (char*)malloc((len + 5) * sizeof(char));
	if (NULL == output_path) {
		printf("Error: pointer is NULL, malloc failed\n");
		free(input_copy);
		return NULL; // error	
	}
	ret = strrchr(input_copy, '/');
	if (NULL == ret) {
		ret = strrchr(input_copy, '\\'); //in case of windows path
		if (NULL == ret)
		{
			printf("Error: Error while trying to find output path\n");
			free(input_copy);
			free(output_path);
			return NULL; // error	
		}
	}
	strcpy(ret + 1, "\0");
	strcpy(output_path, input_copy); // seting the new output path
	strcat(output_path, "Output.txt");

	free(input_copy);
	return output_path;
}