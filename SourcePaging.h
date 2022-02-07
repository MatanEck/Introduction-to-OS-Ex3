#pragma once
//  SourcePaging.h  //
/**********************************************

Authors:
Dvir Katz
Matan Eckhaus Moyal

Project : Ex3 - Paging Table

* *********************************************/
/// Description: This is the declarations module for the project Paging Table. 
///              Header file that contains the functions declarations for the main module to use.

#define _CRT_SECURE_NO_WARNINGS

// Includes:
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <Windows.h>

// Constants:
#define STATUS_SUCCESS  0
#define STATUS_FAILURE  1
#define TRUE 1
#define FALSE 0
#define PAGE_SIZE 4096 // 4KB
#define FRAME_SIZE 4096 // 4KB
#define OFFSET_BITS 12 // 2^12 = 4KB hence the offset in every address is 12 bits
#define PAGE_WAIT 2
#define LRU_INIT -1

// Structs:

// A struct for thread parameters
typedef struct
{
	int frame_number;	// Frame in use
	int valid;			// If the mapping is valid
	int end_of_use;		// End time which mark that we can switch the ended page
	int LRU_count;		// LRU counter, max value will be switched
} page;

// A struct representing the each line from the input file - contains it's relevant data
typedef struct {
	int start_time;		// start time
	int va;			    // virtual address
	int time_of_use;	// use time
} input_line;

// A struct with the relevant arguments for each thread - for thread_main function
typedef struct {
	page* pages;			// List of pages according to the input argument
	input_line input;		// One line argument from the input file
	int* frames;			// list of frames
	int num_of_frames;		// The total number of frames
	int num_of_pages;		// The total number of pages
	HANDLE h_output_file;	// Handle of the output file
} args;

typedef struct
{
	args* argument;
	int thread_index;		// Index of thread
	int num_of_threads;		// Number of total threads

} multi_struct;

HANDLE ghMutex; // global mutex
int curr_time;	// global clock

// Declarations:
static HANDLE create_thread_paging(LPTHREAD_START_ROUTINE p_start_routine,
								   LPVOID p_thread_parameters,
								   LPDWORD p_thread_id);
DWORD WINAPI thread_main(LPVOID lpParam);
extern input_line* read_file_2_input_args(LPSTR file_name, int* p_lines_count);
extern void update_lru(page* p_pages, int num_of_pages);
extern int find_min_end_time(page* p_pages, int num_of_pages);
extern int find_max_end_time(page* p_pages, int num_of_pages);
extern input_line* split_text_file_2_lines_arr(char* source_file, int* p_lines_count);
extern DWORD write_to_file(HANDLE h_output_file, int curr_time, int page_index, int frame_number, char status);
extern int evict_left_pages(page* p_pages, int* frame_arr, int num_of_pages, int num_of_frames, HANDLE h_output_file);
extern int update_current_time(int curr_time, multi_struct* p_args);
extern BOOL check_race_condition(int curr_time, multi_struct* p_args);
extern int free_mem_close_handles(int* p_lines_count, HANDLE* p_thread_handles, HANDLE h_output_file, HANDLE ghMutex,
								  page* thread_pages, int* frame_arr, args* args_for_thread, DWORD* p_thread_ids,
								  input_line* lines_args, multi_struct* multi_arg, char* output_path);
extern char* set_output_path(char* p_input_path);
// Note: see descriptions for the functions in SourcePaging.c