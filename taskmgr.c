#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

enum { TASKMGR_CREATE, TASKMGR_FORK, TASKMGR_DELETE, TASKMGR_INCREASE, TASKMGR_STATUS };
const char* progname = "taskmgr";
const char* semname = "/taskmgr-default";
int value = 1;
int op = TASKMGR_FORK;
const char** commands;
int n_commands;
int fork_times = 1;
int verbose;

#define perror_sem(proc, retval) do {					\
		fprintf(stderr, "%s: %s sem %s failed: %s\n",		\
			__FUNCTION__, proc, semname, strerror(errno));	\
		return retval;						\
	} while (0)

#define perror_create()	perror_sem("create", 3)
#define perror_open()	perror_sem("open", 4)
#define perror_post()	perror_sem("increase", 5)
#define perror_wait()	perror_sem("wait on", 6)
#define perror_unlink()	perror_sem("unlink", 7)
#define perror_getvalue()	perror_sem("get value of", 7)

int taskmgr_create()
{
	sem_t* sem = sem_open(semname, O_RDWR | O_CREAT /* | O_EXCL */, 0666, value);
	if (sem == SEM_FAILED) perror_create();
	sem_close(sem);
	return 0;
}

/* also do decrease */
int taskmgr_increase()
{
	sem_t* sem = sem_open(semname, O_RDWR);
	if (sem == SEM_FAILED) perror_open();
	if (value >= 0) {
		while (value-- > 0) {
			if (sem_post(sem) < 0) perror_post();
		}
	} else {
		while (value++ < 0) {
			if (sem_wait(sem) < 0) perror_wait();
		}
	}
	sem_close(sem);
	return 0;
}

int taskmgr_status()
{
	sem_t* sem = sem_open(semname, O_RDWR);
	if (sem == SEM_FAILED) perror_open();
	if (sem_getvalue(sem, &value) < 0) perror_getvalue();
	sem_close(sem);
	printf("sem %s has %d resources\n", semname, value);
	return 0;
}

int taskmgr_fork()		/* now just run command once */
{
	sem_t* sem = sem_open(semname, O_RDWR);
	if (sem == SEM_FAILED) perror_open();
	if (sem_wait(sem) < 0) perror_wait();
	const char* cmd = *commands;
	if (cmd) {
		if (system(cmd) < 0) {
			fprintf(stderr, "shell command '''%s''' execute failed\n", cmd);
		}
	}
	if (sem_post(sem) < 0) perror_post();
	return 0;
}

int taskmgr_delete()
{
	if (sem_unlink(semname) < 0) perror_unlink();
	return 0;
}

/* parse opt */
void print_args(FILE* out)
{
	switch (op) {
	case TASKMGR_CREATE:
		fprintf(out, "Create taskmgr %s value = %d\n", semname, value); break;
	case TASKMGR_FORK:
		fprintf(out, "Run on taskmgr %s: %d commands each %d times\n", semname, n_commands, fork_times);
		{
			int i;
			for (i = 0; i < n_commands; i++) {
				fprintf(out, "\tCommand '''%s'''\n", commands[i]);
			}
		}
		break;
	case TASKMGR_DELETE:
		fprintf(out, "Delete taskmgr %s\n", semname); break;
	case TASKMGR_INCREASE:
		fprintf(out, "Increase resources of taskmgr %s by %d\n", semname, value); break;
	case TASKMGR_STATUS:
		fprintf(out, "Show status of taskmgr %s\n", semname); break;
	default:
		fprintf(out, "Unknown op %d\n", op);
		exit(1);
	}
	exit(0);
}

void usage(const char* name) {
	fprintf(stderr, "Under the given or default name, run several tasks at a time, limited by resources\n");
	fprintf(stderr, "Create Task:\t%s [-n name] -c resource-count\n", name);
	fprintf(stderr, "Run Process:\t%s [-n name] [-f fork-times] 'shell-command' ...\n", name);
	fprintf(stderr, "Destory Task:\t%s [-n name] -d\n", name);
	fprintf(stderr, "Increase Res:\t%s [-n name] -i resource-count\n", name);
	fprintf(stderr, "Show status:\t%s [-n name] -s\n", name);
}

int parse_opt(int argc, char* argv[])
{
	char** commands_begin;
	static const char* short_options = "+n:c:df:i:sVh";
	static struct option long_options[] = {
		{"name", 1, 0, 'n'},
		{"create", 1, 0, 'c'},
		{"delete", 0, 0, 'd'},
		{"fork", 1, 0, 'f'},
		{"increase", 1, 0, 'i'},
		{"verbose", 0, 0, 'V'},
		{"status", 0, 0, 's'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0}
	};
	progname = argv[0];
	while (1) {
		int c = getopt_long(argc, argv, short_options, long_options, NULL);
		switch (c) {
		case 'n': semname = optarg; break;
		case 'c': op = TASKMGR_CREATE; value = atoi(optarg); break;
		case 'f': fork_times = atoi(optarg); break;
		case 'd': op = TASKMGR_DELETE; break;
		case 'i': op = TASKMGR_INCREASE; value = atoi(optarg); break;
		case 's': op = TASKMGR_STATUS; break;
		case 'V': verbose = 1; break;
		case  -1: goto end_getopt;
		case 'h': default: usage(progname); exit(c == 'h' ? 0 : 1);
		}
	}
end_getopt:
	commands_begin = &argv[optind];
	commands = (const char**)commands_begin;
	n_commands = argc - optind;
	if (verbose) {
		print_args(stderr);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	parse_opt(argc, argv);
	switch (op) {
	case TASKMGR_CREATE:
		return taskmgr_create();
	case TASKMGR_FORK:
		return taskmgr_fork();
	case TASKMGR_DELETE:
		return taskmgr_delete();
	case TASKMGR_INCREASE:
		return taskmgr_increase();
	case TASKMGR_STATUS:
		return taskmgr_status();
	default:
		return 2;
	}
}

