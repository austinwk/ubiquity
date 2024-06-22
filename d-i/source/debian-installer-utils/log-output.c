#define _GNU_SOURCE /* for getopt_long */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <syslog.h>
#include <debian-installer.h>

#ifdef __GNUC__
#  define ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#  define ATTRIBUTE_UNUSED
#endif

/* See below for why this empty handler exists. */
static void sigchld_handler(int signum ATTRIBUTE_UNUSED)
{
}

static int close_orig_stdout(pid_t pid ATTRIBUTE_UNUSED, void *user_data)
{
	int orig_stdout = *(int *) user_data;
	close(orig_stdout);
	return 0;
}

static int restore_orig_stdout(pid_t pid ATTRIBUTE_UNUSED, void *user_data)
{
	int orig_stdout = *(int *) user_data;
	if (dup2(orig_stdout, 1) == -1)
		return 1;
	close(orig_stdout);
	return 0;
}

static int logger(const char *buf, size_t len ATTRIBUTE_UNUSED, void *user_data)
{
	static int log_open = 0;

	if (!log_open) {
		const char *ident = (const char *) user_data;
		if (!ident)
			ident = "log-output";
		openlog(ident, 0, LOG_USER);
		log_open = 1;
	}

	syslog(LOG_NOTICE, "%s", buf);

	return 0;
}

static void usage(FILE *output)
{
	fprintf(output, "Usage: log-output -t TAG [--pass-stdout] PROGRAM [ARGUMENTS]\n");
}

// argv example from ubi-language.Page -> plugin.Plugin -> FilteredCommand.start(...) -> DebconfFilter.start(...)
// "log-output -t ubiquity --pass-stdout /usr/lib/ubiquity/localechooser/localechooser"
int main(int argc, char **argv)
{
	char *tag = NULL;
	static int pass_stdout = 0;

	static struct option long_options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "pass-stdout", no_argument, &pass_stdout, 1 },
		{ NULL, 0, NULL, 0 }
	};

	// Structure describing the action to be taken when a signal arrives.
	struct sigaction sa;

	di_io_handler *stdout_handler = NULL, *stderr_handler = NULL;
	di_process_handler *parent_prepare_handler = NULL;
	di_process_handler *child_prepare_handler = NULL;

	void *prepare_user_data = NULL;
	int orig_stdout = -1;
	int status;

	for (;;) {
		int c = getopt_long(argc, argv, "+t:", long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
			case 0: /* long option */
				break;
			case 'h':
				usage(stdout);
				break;
			case 't':
				tag = strdup(optarg); // tag = "ubiquity"
				break;
			default:
				usage(stderr);
				exit(1);
		}
	}

	// optind = 4 (due to getopt_long iteration)
	if (!argv[optind]) // False
		return 0;

	/* It's possible for subsidiary processes to start daemons which
	 * forget to clean up their file descriptors properly, which means
	 * that polling the other ends of those file descriptors will never
	 * complete.  We install a no-op SIGCHLD handler to make sure that
	 * its poll() gets EINTR and gives up.
	 *
	 * Technically, this is exploiting a bug in di_exec, and a better
	 * solution would be nice ...
	 */
	sa.sa_handler = &sigchld_handler;

	// int sigemptyset(sigset_t *set);
	//   initializes the signal set given by set to empty, with all signals excluded from the set.
	sigemptyset(&sa.sa_mask);

	// #define	SA_NOCLDSTOP  1		 /* Don't send SIGCHLD when children stop.  */
	sa.sa_flags = SA_NOCLDSTOP;

	// int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
	//   used to change the action taken by a process on receipt of a specific signal.  (See signal(7) for an overview of signals.)
	// #define SIGCHLD		17	/* Child terminated or stopped.  */
	sigaction(SIGCHLD, &sa, NULL);

	if (pass_stdout) { // True
		orig_stdout = dup(1); // File descriptor 1 is stdout
		parent_prepare_handler = &close_orig_stdout;
		child_prepare_handler = &restore_orig_stdout;
		prepare_user_data = &orig_stdout;
	} else
		stdout_handler = &logger;

	stderr_handler = &logger;

	// int di_exec_path_full (const char *file, const char *const argv[], di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_process_handler *parent_prepare_handler, void *parent_prepare_user_data, di_process_handler *child_prepare_handler, void *child_prepare_user_data);
	// execvp like call: exec() replaces the current process image with a new process image
	//
	// file: executable
	// argv: NULL-terminated area of char pointer
	// stdout_handler: di_io_handler which gets stdout (and to stderr if stderr_handler is NULL)
	// stderr_handler: di_io_handler which gets stderr
	// io_user_data: user_data for di_io_handler
	// parent_prepare_handler: di_process_handler which is called after the fork in the parent
	// parent_prepare_user_data: user_data for parent_prepare_handler
	// child_prepare_handler: di_process_handler which is called after the fork in the child
	// child_prepare_user_data: user_data for child_prepare_handler
	// return: status or error

	// argv[optind] = "/usr/lib/ubiquity/localechooser/localechooser"
	status = di_exec_path_full(argv[optind], (const char **) &argv[optind],
		stdout_handler, stderr_handler, tag,
		parent_prepare_handler, prepare_user_data,
		child_prepare_handler, prepare_user_data);

	return di_exec_mangle_status(status);
}
