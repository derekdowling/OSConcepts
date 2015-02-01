// Profiles the processes main memory

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <signal.h>

#include "memchunk.h"

/* Sets our current memchunk for signhandler use */
static jmp_buf env;

/**
 * Parses and groups all consecutive memory chunks based on their "RW" struct
 * attributes.
 *
 * @return total number of consecutively grouped read/write permissions memory
 * chunks
 */
int get_mem_layout(struct memchunk * chunk_list, int size)
{
	/* set base counters and such */
	int page_size = sysconf(_SC_PAGESIZE);
	int list_size = 0;

	/* includes the initial chunk */
	int total_chunks = 1;

	/* Tracks Permissions */
	int last_permission;
	int current_permission;

	/* For checking addresses */
	char* current_addr = (char*) 0;
	char* last_addr = (char*) 0;
	char* chunk_start_addr = 0;

	last_permission = get_rw(current_addr);

	/* iterate until we wrap around back to 0 */
	while (current_addr >= last_addr) {

		current_permission = get_rw(current_addr);

		/* If our latest chunk has a different access mode, increment and
		* update */
		if (current_permission != last_permission) {

			/* populate our chunk_list if there is still room */
			if (list_size < size) {
				chunk_list[list_size].start = chunk_start_addr;
				chunk_list[list_size].length = current_addr - chunk_start_addr;
				chunk_list[list_size].RW = last_permission;
				list_size++;
			}

			/* increment chunk count and set new permission */
			total_chunks++;
			last_permission = current_permission;
			chunk_start_addr = current_addr;
		}

		/* increment trackers */
		last_addr = current_addr;
		current_addr += page_size;
	}

	/* adds the last chunk to the list if still room */
	if (list_size < size) {
		chunk_list[list_size].start = chunk_start_addr;
		chunk_list[list_size].length = current_addr - chunk_start_addr;
		chunk_list[list_size].RW = last_permission;
		list_size++;
	}

	return total_chunks;
}

/**
 * Tests a page in memory to see what it's user level permissions are.
 */
int get_rw(char* current_addr)
{
	if (can_read(current_addr)) {

		if (can_write(current_addr)) {
			/* "1" means read & write perms */
			return 1;
		}

		/* "0" means read only */
		return 0;
	}

	/* Defaults to no permissions via "-1" */
	return -1;
}

/**
 * Checks whether or not a memory block is readable. Returns true(1) if so.
 */
int can_read(char *ptr)
{
	char temp;

	/* set our signal handler */
	(void) signal(SIGSEGV, sigsegv_handler);

	if (setjmp(env)) {
		return 0;
	}

	temp = *ptr;
	return 1;
}

/**
 * Checks whether or not a memory block is writable. Returns true(1) if so.
 * Assumes that the memory block is readable to succeed.
 */
int can_write(char *ptr)
{
	char temp;

	/* set our signal handler */
	(void) signal(SIGSEGV, sigsegv_handler);

	if (setjmp(env)) {
		return 0;
	}

	/* Read, then write. Don't change ptr value otherwise bad things might */
	/* happen here */
	temp = *ptr;
	*ptr = temp;

	return 1;
}

/**
 * Handles any Seg Fault Signals caused by accessing invalid memory
 * within the app.
 */
void sigsegv_handler(int sig)
{
	longjmp(env, 1);
}
