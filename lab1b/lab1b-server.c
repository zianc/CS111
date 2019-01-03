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
#include <sys/socket.h>
#include <netinet/in.h>
#include <mcrypt.h>


int port_flag, encrypt_flag, log_flag, debug_flag; // flags for command line arguments

int is_pipe_closed; // record if the write end of pipe to shell has been closed
int new_socket_fd; // record fd of the socket connected to client
pid_t child; // for parent store the child_pid and for child, store 0
int pipeid_to_shell[2];
int pipeid_from_shell[2];  

// for --encrypt
MCRYPT crypt_fd, decrypt_fd;
char* key; // key used for encryption
char* iv;  // initialization vector


void process_keyboard_input(); // read from client and forward to shell
void process_shell_input(); // read from shell and forward to client
void harvest(); // shutdown the server

void sighandler(int signum);  // catch the SIGPIPE fault when write to the child

// a helper function that reports an error message and exit 1
void report_error_and_exit(const char* msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

int main(int argc , char* argv[])
{
	port_flag = 0;
	debug_flag = 0;
	encrypt_flag = 0;
	char* server_portno;
	char* key_filename;
	char* shell_filename = "/bin/bash";
	char* usage = "Correct usage: lab1a --port [--encrypt][--debug]\n";

	// get the optional arguments
	static const struct option long_options[] = 
	{
		{"port", required_argument, NULL, 'p'},
		{"encrypt", required_argument, NULL, 'e'},
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
			case 'p':      
				port_flag = 1;
				server_portno = optarg;
				break;  // for --shell flag
			case 'e':
				encrypt_flag = 1;
				key_filename = optarg;
				break;
			case 'd':
				debug_flag = 1;
				break;  // for --debug flag
			default:    //case '?':
				fprintf(stderr, "%s\n", usage);
				exit(1);  // for unrecognized argument
		}

	}

	int server_socket_fd;
	int portno, backlog_len;
	unsigned client_addr_len;
	struct sockaddr_in server_addr, client_addr;

	if (! port_flag) // check --port argument
	{
		fprintf(stderr, "Error: no port provided.\n");
		fprintf(stderr, "%s\n", usage);
		exit(1);
	} 

	portno = atoi(server_portno);
	// create the socker
	server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket_fd < 0)
		report_error_and_exit("Error creating socket in the server");
	// fill in the server address
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portno);
	// bind the socket to the server address
	if (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		report_error_and_exit("Error binding the socket in the server");
	// let the server socket listen for connections
	backlog_len = 5; // the maximum number of pending connections
	listen(server_socket_fd, backlog_len);
	client_addr_len = sizeof(client_addr);
	new_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
	// block until a connection from the client

	if (new_socket_fd < 0)
		report_error_and_exit("Error accepting in the server");

	close(server_socket_fd);
	// no longer need the listening socket

	// create one pipe from shell to terminal and another pipe from terminal to shell
	if (pipe(pipeid_to_shell) < 0)
		report_error_and_exit("Error creating pipe from server to shell");

	if (pipe(pipeid_from_shell) < 0)
		report_error_and_exit("Error creating pipe from shell to server");

	child = fork();
	if (child < 0)
		report_error_and_exit("Error forking a child for the shell");

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

	else  // the parent process
	{

		close(pipeid_to_shell[0]);
		close(pipeid_from_shell[1]);  // close unused ends of pipes

		int key_fd;
		int keylen;
		struct stat keyfile_stat;
		int iv_size;

		if (encrypt_flag)
		{

		key_fd = open(key_filename, O_RDONLY);
		if (key_fd < 0)
			report_error_and_exit("Error opening keyfile");

		// retrive the key from keyfile
		if(fstat(key_fd, &keyfile_stat) < 0) 
			report_error_and_exit("Error getting the file status of keyfile");
		keylen = keyfile_stat.st_size;
		key = (char*) malloc(keylen);
		if(read(key_fd, key, keylen) < 0)
			report_error_and_exit("Error reading from keyfile");

		crypt_fd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  		if (crypt_fd == MCRYPT_FAILED)
  			report_error_and_exit("Error opening encrypt module in the client");
  		decrypt_fd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  		if (decrypt_fd == MCRYPT_FAILED)
  			report_error_and_exit("Error opening encrypt module in the client");

  		iv_size = mcrypt_enc_get_iv_size(crypt_fd);
  		iv = (char*) malloc(iv_size);
  		int j;
  		for (j = 0; j < iv_size; ++j)
  			iv[j] = '\0';

  		j = mcrypt_generic_init(crypt_fd, key, keylen, iv);
  		if (j < 0)
  			report_error_and_exit("Error initializing encrypt module");
  		j = mcrypt_generic_init(decrypt_fd, key, keylen, iv);
  		if (j < 0)
  			report_error_and_exit("Error initializing decrypt module");

		}

		int poll_ret;
		int timeout = -1; // blocks until some inputs have occured

		is_pipe_closed = 0;

		struct pollfd input_fds[] =  // define the two input channels we are multiplexing
		{
			{new_socket_fd, POLLIN, 0},
			{pipeid_from_shell[0], POLLIN, 0}
		};

		atexit(harvest);

		while (1)
		{
			poll_ret = poll(input_fds, 2, timeout);
			if (poll_ret < 0)
				report_error_and_exit("Error polling the input channels in the server");

			if (poll_ret > 0)
				// if some input events have occured
			{
				if (input_fds[0].revents & POLLIN)
					process_keyboard_input();
				if (input_fds[1].revents & POLLIN)
					process_shell_input();
				if (input_fds[0].revents & (POLLHUP | POLLERR))
				{
					// receive a read error from client socket
					if (! is_pipe_closed)
					{
						close(pipeid_to_shell[1]);
						is_pipe_closed = 1;
					}
				}				
				if (input_fds[1].revents & (POLLHUP | POLLERR))
				{
					exit(0);
				}

			}
		} // end of poll

	}// end of parent process
	exit(0);
}


// read from client and forward to shell
void process_keyboard_input()
{
	const int MAX_BYTES = 64;
	char buffer[MAX_BYTES];
	char client_buffer[MAX_BYTES];

	int nread = read(new_socket_fd, client_buffer, MAX_BYTES);
	if (nread < 0)
		report_error_and_exit("Error reading from socket in the server");

	bcopy(client_buffer, buffer, nread);
	if (encrypt_flag)
		mdecrypt_generic(decrypt_fd, buffer, nread);
		// decrypt the data from client

	int i;
	for (i = 0; i < nread; ++i)
	{
		char ch = buffer[i];
		if (ch == '\r')
		{
			if(write(pipeid_to_shell[1], "\n", 1) < 0)
				report_error_and_exit("Error writing to shell in the server");
		}

		else if (ch == 0x04)  // receive ^D 
		{
			if (! is_pipe_closed)
			{
				close(pipeid_to_shell[1]); 
				is_pipe_closed = 1; // close the write pipe to shell
			}
		}
		else if (ch == 0x03)  // receive ^C
		{
			if (kill(child, SIGINT) < 0)
				report_error_and_exit("Failed to send SIGINT to shell"); // send an interrupt signal to shell
		}
		else
		{
			if(write(pipeid_to_shell[1], &ch, 1) < 0)
				report_error_and_exit("Error writing to shell in the server");
		}
	}

}	


// read from shell and forward to client
void process_shell_input()
{
	const int MAX_BYTES = 256;
	char buffer[MAX_BYTES];
	char temp[2];

	int nread = read(pipeid_from_shell[0], buffer, MAX_BYTES);
	if (nread < 0)
		report_error_and_exit("Error reading from the shell");

	int i;
	for (i = 0; i < nread; ++i)
	{
		char ch = buffer[i];
		if (ch == '\n')
		{
			temp[0] = '\r';
			temp[1] = '\n';
			if (encrypt_flag)
				mcrypt_generic(crypt_fd, temp, 2);
			if(write(new_socket_fd, temp, 2) < 0)
				report_error_and_exit("Error writing to socket in the server");
		}

		else if (ch == 0x04)  // receive ^D from shell
		{
			exit(0);
		}

		else
		{
			temp[0] = ch;
			if (encrypt_flag)
				mcrypt_generic(crypt_fd, temp, 1);
			if(write(new_socket_fd, temp, 1) < 0)
				report_error_and_exit("Error writing to socket in the server"); 
		}
	}


}

// harvest the exit status of the shell, close socket to the client, and exit
void harvest()
{
	int child_status;
	if (waitpid(child, &child_status, 0) < 0)
		report_error_and_exit("Error waiting for the child process");
	
	// report the exit status of child
	int signal_code = child_status & 0x7f;
	int status_code = WEXITSTATUS(child_status);
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", signal_code, status_code);
	// close the network socket to the client

	if (encrypt_flag)
	{
		free(iv);
		free(key);
		mcrypt_generic_deinit(crypt_fd);
  		mcrypt_module_close(crypt_fd);
  		mcrypt_generic_deinit (decrypt_fd);
  		mcrypt_module_close(decrypt_fd);
	}

	close(new_socket_fd);
}

void sighandler(int signum)  // catch the SIGPIPE fault which might occur at the final poll
{
	if (signum == SIGPIPE)
	{	
		if (debug_flag)
			fprintf(stderr, "The shell has already shut down.\n"); 
		exit(0);
	}
}









