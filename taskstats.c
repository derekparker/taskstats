#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <mach/mach.h>

void
acquire_task_for_pid(const char *pidStr, mach_port_name_t *task) {
	char *endPtr;
	errno = 0;
	kern_return_t kret;

	long int pid = strtol(pidStr, &endPtr, 10);
	if (errno == EINVAL) {
		printf("Invalid pid %s\n", pidStr);
		exit(1);
	}

	kret = task_for_pid(mach_task_self(), pid, task);
	if (kret != KERN_SUCCESS) {
		printf("Could not acquire task port: %s\n", mach_error_string(kret));
		exit(1);
	}
	printf("Pid: %ld\n", pid);
}

/*
 * Print information about mach threads for
 * associated task.
 * Struct values (/usr/include/mach/thread_info.h):
	time_value_t    user_time;     
	time_value_t    system_time;   
	integer_t       cpu_usage;     
	policy_t	policy;	       
	integer_t       run_state;     
	integer_t       flags;         
	integer_t       suspend_count; 
	integer_t       sleep_time;    
 * */
void
print_thread_info(mach_port_name_t task) {
	kern_return_t kret;
	thread_act_array_t list;
	mach_msg_type_number_t count;

	kret = task_threads(task, &list, &count);
	if (kret != KERN_SUCCESS) {
		printf("Could not get task threads %s\n", mach_error_string(kret));
		exit(1);
	}

	puts("Thread Id\tSuspend Count\t");
	for (int i = 0; i < count; i++) {
		struct thread_basic_info info;
		unsigned int info_count = THREAD_BASIC_INFO_COUNT;
		kret = thread_info((thread_t)list[i], THREAD_BASIC_INFO, (thread_info_t)&info, &info_count);
		if (kret != KERN_SUCCESS) {
			printf("Could not get thread info %s\n", mach_error_string(kret));
			exit(1);
		}
		printf("%d\t\t%d\n", list[i], info.suspend_count);
	}
}

int
main(const int argc, const char **argv) {
	if (getuid() != 0) {
		puts("Must be root to execute this program.");
		return 1;
	}
	if (argc == 1) {
		puts("You must provide pid as argument.");
		return 1;
	}
	// Get the task port for $PID.
	mach_port_name_t task;
	acquire_task_for_pid(argv[1], &task);
	print_thread_info(task);
	return 0;
}
