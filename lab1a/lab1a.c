
// NAME: Haiying Huang
// EMAIL: hhaiying1998@outlook.com
// ID: 804757410
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>

struct termios saved_attr; // store the original mode paramters
int shell_flag, debug_flag;
int is_ready_to_exit, is_pipe_closed; // record if it is time to terminate the parent process
pid_t child;    // for parent store the child_pid and for child, store 0
int pipeid_to_shell[2];
int pipeid_from_shell[2];  // two pipes for communication between parent and child

void set_terminal_mode(); // set noncanonical mode without echo for stdin
void restore_terminal_mode(); // restore original terminal mode
void keep_echo_to_terminal(); // keep echoing to terminal in nonshell mode
void write_and_check(int fd, const char* buf, size_t count); // write with failure check
void sighandler(int signum);  // catch the SIGPIPE fault which might occur at the final poll
// read from shell and forward to terminal
void process_shell_input();
// read from terminal, echo to terminal, and forward to shell
void process_keyboard_input();

int main(int argc, char* argv[])
{

	shell_flag = 0;
	debug_flag = 0;
	char* shell_filename;  
	// support two types of long optional arguments
	static const struct option long_options[] = 
	{
		{"shell", required_argument, NULL, 's'},
		{"debug", no_argument, NULL, 'd'},
		{0, 0, 0, 0}
	};

	char c;
	while (1)
	{
		int option_index = 0;  // store the index of the next option we find
		c = getopt_long(argc, argv, "", long_options, &option_index); // get the next option

		if (c == -1)
			break;

		switch(c)
		{
			case 's':      
				shell_flag = 1;
				shell_filename = optarg;
				break;  // for --shell flag
			case 'd':
				debug_flag = 1;
				break;  // for --debug flag
			default:    //case '?':
				fprintf(stderr, "Correct usage: lab1a [--shell program_name] [--debug]\n");
				exit(1);  // for unrecognized argument
		}

	}

	set_terminal_mode(); // set the noncanonical mode with no echo for stdin
	if (!shell_flag)
	{
		keep_echo_to_terminal();
		if (debug_flag)
			fprintf(stderr, "We should not arrive here in nonshell mode.\n");
	}

	// for shell mode
	signal(SIGPIPE, sighandler);

	// create one pipe from shell to terminal and another pipe from terminal to shell
	if (pipe(pipeid_to_shell) < 0)
	{
		fprintf(stderr, "Error creating pipe from terminal process to shell: %s\n", strerror(errno));
		exit(1);
	}

	if (pipe(pipeid_from_shell) < 0)
	{	
		fprintf(stderr, "Error creating pipe from shell process to terminal: %s\n", strerror(errno));
		exit(1);
	}

	child = fork();
	if (child < 0)
	{
		fprintf(stderr, "Error forking a new process: %s\n", strerror(errno));
		exit (1);
	}
	else if (child == 0)  
		// the child process
	{
		close(pipeid_to_shell[1]);  // close the write end of the pipe from terminal
		close(pipeid_from_shell[0]);  // close the read end of the pipe from shell

		close(STDIN_FILENO);
		dup(pipeid_to_shell[0]); 
		close(pipeid_to_shell[0]);   // connect the pipe from the terminal to stdin 

		close(STDOUT_FILENO);
		dup(pipeid_from_shell[1]);     // connect the stdout and the stderr to the pipe to the terminal
		close(STDERR_FILENO);
		dup(pipeid_from_shell[1]);
		close(pipeid_from_shell[1]);

		execl(shell_filename, shell_filename, (char*) NULL);  // make the child process run the particular program
		fprintf(stderr, "Error executing shell in the child process: %s\n", strerror(errno));
		exit(1);
	}

	else
		// the parent process
	{
		is_pipe_closed = 0;
		is_ready_to_exit = 0;
		close(pipeid_to_shell[0]);
		close(pipeid_from_shell[1]);  // close unused ends of pipes

		int poll_ret;
		int child_status;
		int timeout = -1; // blocks until some inputs have occured

		struct pollfd input_fds[] =  // define the two input channels we are multiplexing
		{
			{STDIN_FILENO, POLLIN, 0},
			{pipeid_from_shell[0], POLLIN, 0}
		};

		while (1)
		{
			poll_ret = poll(input_fds, 2, timeout);
			if (poll_ret < 0)
			{
				fprintf(stderr, "Error polling the input channels: %s\n", strerror(errno));
				exit (1);
			}

			if (poll_ret > 0)
				// if some input events have occured
			{
				if (input_fds[0].revents & POLLIN)
					process_keyboard_input();
				if (input_fds[1].revents & POLLIN)
					process_shell_input();
				if (input_fds[1].revents & (POLLHUP | POLLERR))
				{
					is_ready_to_exit = 1;
					// break out the poll
					if (debug_flag)
						fprintf(stderr, "shell hangup.\n");
				}
				if (input_fds[0].revents & (POLLHUP | POLLERR))
				{
					is_ready_to_exit = 1;
					// break out the poll
					if (debug_flag)
						fprintf(stderr, "stdin hangup.\n");
				}

			}

			if (is_ready_to_exit)
				break;
		} // end of poll

		if (!is_pipe_closed)
			close(pipeid_to_shell[1]);

		if (waitpid(child, &child_status, 0) < 0)
		{
			fprintf(stderr, "Error waiting child process: %s\n", strerror(errno));
			exit (1);
		}

		// report the exit status of child
		int signal_code = child_status & 0x7f;
		int status_code = WEXITSTATUS(child_status);
		fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", signal_code, status_code);
		exit(0);

	}
}

void set_terminal_mode()
{
	struct termios temp_attr; 
	if (tcgetattr(STDIN_FILENO, &temp_attr) < 0)
	{
		fprintf(stderr, "Error getting original mode: %s\n", strerror(errno));
		exit(1);
	}

	saved_attr = temp_attr;
	temp_attr.c_iflag = ISTRIP; // set noncanonical with no echo mode
	temp_attr.c_oflag = 0;
	temp_attr.c_lflag = 0;

	if (tcsetattr(STDIN_FILENO, TCSANOW, &temp_attr) < 0)
	{
		fprintf(stderr, "Error setting noncanonical: %s\n", strerror(errno));
		exit(1);
	}

	atexit(restore_terminal_mode); // restore the original mode every time before exit

}

void restore_terminal_mode()
{
	if(tcsetattr(STDIN_FILENO, TCSANOW, &saved_attr) < 0)
	{
		fprintf(stderr, "Error restoring original mode of terminal: %s\n", strerror(errno));
		_exit(1);
	}
}

void write_and_check(int fd, const char* buf, size_t count)
{
	if (write(fd, buf, count) < 0)
	{
		if (fd == STDOUT_FILENO)
			fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
		else
			fprintf(stderr, "Error writing to file %d: %s\n", fd, strerror(errno));
		exit(1);
	}
}
// a helper function that writes to fd and checks syscall failure


// keep echoing to terminal in nonshell mode
void keep_echo_to_terminal()
{
	const int MAX_BYTES = 8;
	char buffer[MAX_BYTES];

	int count;
	while (1)
	{
		count = read(STDIN_FILENO, buffer, MAX_BYTES);
		if (count < 0)
		{
			fprintf(stderr, "Error reading characters from terminal: %s\n", strerror(errno));
			exit(1);
		}

		int i;
		for (i = 0; i < count; ++i)
		{
			char ch = buffer[i];
			char temp[2];
			if (ch == '\r' || ch == '\n')
			{
				temp[0] = '\r';
				temp[1] = '\n';
				write_and_check(STDOUT_FILENO, temp, 2);
			}
			else if (ch == 0x04)  // receive an eof 
			{
				temp[0] = '^';
				temp[1] = 'D';
				write_and_check(STDOUT_FILENO, temp, 2);
				exit(0);
			}
			else
				write_and_check(STDOUT_FILENO, &ch, 1);
		}
	}
}

void sighandler(int signum)  // catch the SIGPIPE fault which might occur at the final poll
{
	if (signum == SIGPIPE)
	{	
		if (debug_flag)
			fprintf(stderr, "receive SIGPIEP.\n"); 
		is_ready_to_exit = 1;
	}
}

// read from terminal, echo to terminal, and forward to shell
void process_keyboard_input()
{
	const int MAX_BYTES = 8;
	char buffer[MAX_BYTES];

	int count = read(STDIN_FILENO, buffer, MAX_BYTES);
	if (count < 0)
	{
		fprintf(stderr, "Error reading from terminal: %s\n", strerror(errno));
		exit(1);
	}

	int i;
	for (i = 0; i < count; ++i)
	{
		char ch = buffer[i];
		char temp[2];
		if (ch == '\r' || ch == '\n')
		{
			temp[0] = '\r';
			temp[1] = '\n';
			write_and_check(STDOUT_FILENO, temp, 2);
			write_and_check(pipeid_to_shell[1], temp + 1, 1);
		}
		else if (ch == 0x04)  // receive an eof 
		{
			temp[0] = '^';
			temp[1] = 'D';
			write_and_check(STDOUT_FILENO, temp, 2);
			if (! is_pipe_closed)
			{
				close(pipeid_to_shell[1]);
				is_pipe_closed = 1;
			}
			if (debug_flag)
				fprintf(stderr, "eof on stdin.\n");
		}
		else if (ch == 0x03)  // receive an interrupt
		{
			temp[0] = '^';
			temp[1] = 'C';
			write_and_check(STDOUT_FILENO, temp, 2);
			if (kill(child, SIGINT) < 0)
			{
				fprintf(stderr, "Error sending SIGINT to shell: %s\n", strerror(errno));
				exit(1);
			}
		}

		else
		{
			write_and_check(STDOUT_FILENO, &ch, 1);
			write_and_check(pipeid_to_shell[1], &ch, 1);
			// echo and forward a character at a time
		}
	}

}	


// read from shell and forward to terminal
void process_shell_input()
{
	const int MAX_BYTES = 256;
	char buffer[MAX_BYTES];

	int count = read(pipeid_from_shell[0], buffer, MAX_BYTES);
	if (count < 0)
	{
		fprintf(stderr, "Error reading from the shell: %s\n", strerror(errno));
		exit(1);
	}

	int i;
	for (i = 0; i < count; ++i)
	{
		char ch = buffer[i];
		char temp[2];
		if (ch == '\n')
		{
			temp[0] = '\r';
			temp[1] = '\n';
			write_and_check(STDOUT_FILENO, temp, 2);
		}

		else if (ch == 0x04)  // receive an eof from shell
		{
			temp[0] = '^';
			temp[1] = 'D';
			write_and_check(STDOUT_FILENO, temp, 2);
			is_ready_to_exit = 1;
			if (debug_flag)
				fprintf(stderr, "eof from shell.\n");
		}

		else
			write_and_check(STDOUT_FILENO, &ch, 1); 
			// foreward to stdout one character at a time
	}
}
