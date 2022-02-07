/**********************************************

Authors:
Dvir Katz
Matan Eckhaus Moyal

Project : Ex3 - Paging Table

* *********************************************/

/// Description: This is the main module for the project Paging Table.
///				 Simulates paging table that maps pages to their frames.


// Includes:
#include "SourcePaging.h"


/**************************************** Main Function Summary: ****************************************
/// Description: This program will simulate paging table that maps pages to their frames.
///
/// Parameters:
///		argc - int. the number of input arguments - should accept 4 arguments.
///		argv - char pointer to a list of input arguments - path to executable, number of bits in virtual memory,
///														   number of bits in physical memory, path to input file
///	Returns: int value - 0 if succeeded, 1 if failed
*********************************************************************************************************/
int main(int argc, char* argv[])
{
	// Check if the input is correct (4  arguments)
	if (4 != argc) {
		printf("Error: Wrong number of input arguments!\n");
		return STATUS_FAILURE;
	}
	int i = 0, exit_code_val = 0, p_lines_count = 0, ret_val = 0;
	input_line* lines_args = NULL;
	curr_time = 0;
	int num_bits_virtual_mem = atoi(argv[1]);
	int num_bits_pysical_mem = atoi(argv[2]);
	LPSTR input_file_name = argv[3];
	int vm_num_index_bits = num_bits_virtual_mem - OFFSET_BITS;
	int pm_num_index_bits = num_bits_pysical_mem - OFFSET_BITS;
	int num_of_pages = pow(2, vm_num_index_bits);
	int num_of_frames = pow(2, pm_num_index_bits);

	lines_args = read_file_2_input_args(input_file_name, &p_lines_count);
	if (lines_args == NULL) {
		printf("Error: Failed to read file\n");
		return STATUS_FAILURE;
	}

	HANDLE* p_thread_handles = (HANDLE*)malloc(p_lines_count * sizeof(HANDLE));
	if (p_thread_handles == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = free_mem_close_handles(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, lines_args, NULL, NULL);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	DWORD* p_thread_ids = (DWORD*)malloc(p_lines_count * sizeof(DWORD));
	if (p_thread_ids == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, NULL, NULL, NULL, NULL, NULL, NULL, lines_args, NULL, NULL);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	page* thread_pages = NULL;
	thread_pages = (page*)malloc(num_of_pages * sizeof(page));
	if (thread_pages == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, NULL, NULL, NULL, NULL, NULL, p_thread_ids,
			lines_args, NULL, NULL);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	for (i = 0; i < num_of_pages; i++) {		//  ** init for pages: **
		thread_pages[i].valid = 0;				// means that page is not mapped to frame
		thread_pages[i].LRU_count = -2;			// set init value for LRU counter
	}
	int* frame_arr = NULL;
	frame_arr = (int*)malloc(sizeof(int) * num_of_frames);
	if (frame_arr == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, NULL, NULL, thread_pages,
			NULL, NULL, p_thread_ids, lines_args, NULL, NULL);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	for (int i = 0; i < num_of_frames; i++) {	// ** init for frames: **
		frame_arr[i] = 0;  // init frame (0 = no page is mapped / 1 = page is mapped to frame)
	}

	// creating handle for the output file
	LPSTR output_path = set_output_path(input_file_name);
	if (output_path == NULL) {
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, NULL, NULL, thread_pages,
			frame_arr, NULL, p_thread_ids, lines_args, NULL, NULL);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	HANDLE h_output_file = CreateFileA(output_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h_output_file == INVALID_HANDLE_VALUE) {	// If failed to open file
		printf("Error: Failed to open file! The error code is %d\n", GetLastError());
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, NULL, NULL, thread_pages,
			frame_arr, NULL, p_thread_ids, lines_args, NULL, output_path);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	args* args_for_thread = NULL;
	args_for_thread = (args*)malloc(p_lines_count * sizeof(args));
	if (args_for_thread == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, h_output_file, NULL, thread_pages,
			frame_arr, NULL, p_thread_ids, lines_args, NULL, output_path);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	for (i = 0; i < p_lines_count; i++) {
		args_for_thread[i].input = lines_args[i];
		args_for_thread[i].pages = thread_pages;
		args_for_thread[i].frames = frame_arr;
		args_for_thread[i].num_of_frames = num_of_frames;
		args_for_thread[i].num_of_pages = num_of_pages;
		args_for_thread[i].h_output_file = h_output_file;
	}
	multi_struct* multi_arg = NULL;
	multi_arg = (multi_struct*)malloc(p_lines_count * sizeof(multi_struct));
	if (multi_arg == NULL) {
		printf("Error: Failed to allocate memory\n");
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, h_output_file, NULL, thread_pages,
			frame_arr, args_for_thread, p_thread_ids, lines_args, NULL, output_path);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	for (i = 0; i < p_lines_count; i++) {
		multi_arg[i].argument = args_for_thread;
		multi_arg[i].num_of_threads = p_lines_count;
		multi_arg[i].thread_index = i;
	}

	ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex
	if (ghMutex == NULL) {
		printf("Error: Create Mutex error, the error is: %d\n", GetLastError());
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, h_output_file, NULL, thread_pages,
			frame_arr, args_for_thread, p_thread_ids, lines_args, multi_arg, output_path);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//										START paging according to the input file
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (i = 0; i < p_lines_count; i++) {
		p_thread_handles[i] = create_thread_paging(thread_main, &multi_arg[i], &p_thread_ids[i]);
		if (p_thread_handles[i] == NULL) {
			printf("Error when creating thread\n");
			ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, h_output_file, ghMutex, thread_pages,
				frame_arr, args_for_thread, p_thread_ids, lines_args, multi_arg, output_path);
			if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
			return STATUS_FAILURE;
		}
	}
	WaitForMultipleObjects(p_lines_count, p_thread_handles, TRUE, INFINITE);
	ret_val = evict_left_pages(thread_pages, frame_arr, num_of_pages, num_of_frames, h_output_file);
	if (ret_val == STATUS_FAILURE) {
		printf("Error: Error while evicting the left pages from frames\n");
		ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, h_output_file, ghMutex, thread_pages,
			frame_arr, args_for_thread, p_thread_ids, lines_args, multi_arg, output_path);
		if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }
		return STATUS_FAILURE;
	}
	//////// Done - now free memory allocations and close handles ///////////
	ret_val = free_mem_close_handles(&p_lines_count, p_thread_handles, h_output_file, ghMutex, thread_pages,
		frame_arr, args_for_thread, p_thread_ids, lines_args, multi_arg, output_path);
	if (ret_val == STATUS_FAILURE) { return STATUS_FAILURE; }

	return STATUS_SUCCESS;
}



/// === create_thread_paging ===
/// Description: This function creates thread that should run the thread_main function.
/// Parameters:
///		p_start_routine - a pointer to the function the thread runs (thread_main).
///		p_thread_parameters - void pointer to with the relevant parameters that the function needs.
///		p_thread_id -  int pointer for the thread's ID
///	Returns: Handle to thread if succeeded, Null if fails
static HANDLE create_thread_paging(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters,
	LPDWORD p_thread_id)
{
	HANDLE thread_handle;

	if (NULL == p_start_routine) {
		printf("Error when creating a thread. Received null pointer\n");
		return NULL;
	}

	if (NULL == p_thread_id) {
		printf("Error when creating a thread. Received null pointer\n");
		return NULL;
	}

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_start_routine,     /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		p_thread_id);        /*  returns the thread identifier */

	return thread_handle;
}