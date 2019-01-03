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
#include <netdb.h>
#include <sys/stat.h>
#include <mcrypt.h>

struct termios saved_attr; // store the original mode paramters
int port_flag, encrypt_flag, log_flag, debug_flag;
int client_socket_fd;

pid_t child;    // for parent store the child_pid and for child, store 0
int pipeid_to_shell[2];
int pipeid_from_shell[2];  // two pipes for communication between parent and child

// for --encrypt
MCRYPT crypt_fd, decrypt_fd;
char* key; // key used for encryption
char* iv;  // initialization vector


void set_terminal_mode(); // set noncanonical mode without echo for stdin
void restore_terminal_mode(); // restore original terminal mode

// a helper function that reports an error message and exit 1
void report_error_and_exit(char* msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

int main(int argc, char* argv[])
{
	port_flag = 0;
	debug_flag = 0;
	encrypt_flag = 0;
	log_flag = 0;
	char* server_portno;
	char* key_filename;
	char* log_filename;
	char* usage = "Correct usage: lab1a --port [--encrypt][--log][--debug]\n";

	// get the optional arguments
	static const struct option long_options[] = 
	{
		{"port", required_argument, NULL, 'p'},
		{"encrypt", required_argument, NULL, 'e'},
		{"log", required_argument, NULL, 'l'},
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
			case 'l':
				log_flag = 1;
				log_filename = optarg;
				break;
			case 'd':
				debug_flag = 1;
				break;  // for --debug flag
			default:    //case '?':
				fprintf(stderr, "%s\n", usage);
				exit(1);  // for unrecognized argument
		}

	}

	// get the optional arguments

	struct sockaddr_in server_addr;
	struct hostent* server_host;
	int portno;

	if (! port_flag)
	{
		fprintf(stderr, "Error: no port provided.\n");
		fprintf(stderr, "%s\n", usage);
		exit(1);
	} 


	set_terminal_mode(); // set noncanonical input mode for stdin

	portno = atoi(server_portno); // get the port number of the server
	// create the socket
	client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket_fd < 0)
		report_error_and_exit("Error creating socket in the client");
	// get the host information of the server
	server_host = gethostbyname("localhost");
	if (!server_host)
		report_error_and_exit("Error getting server host information");
	// fill in the server address
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	bcopy((char *)(server_host -> h_addr), (char *)&server_addr.sin_addr.s_addr, server_host->h_length);
	server_addr.sin_port = htons(portno);
	// connect to the server by its address
	if (connect(client_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
			report_error_and_exit("Fail to connect to the server");

	int poll_ret;
	int nread;
	int timeout = -1;

	const int MAX_BYTES = 64;
	char buf[MAX_BYTES];
	char server_buf[MAX_BYTES];


	struct pollfd input_fds[] = // polling between STDIN and sockets from the server
	{
		{STDIN_FILENO, POLLIN, 0},
		{client_socket_fd, POLLIN, 0}
	};

    int log_fd; // keep a log file for data read from and write to the server
	if (log_flag)
	{
		log_fd = creat(log_filename, 0666);
		if (log_fd < 0)
			report_error_and_exit("Error opening the log file");
	}

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



	while (1)
	{
		poll_ret = poll(input_fds, 2, timeout);
		if (poll_ret < 0)
			report_error_and_exit("Error polling input channels in the client");
		if (poll_ret > 0) 
			// some input events occur
		{
			if(input_fds[0].revents & POLLIN) // receiving commands from stdin
			{
				nread = read(STDIN_FILENO, buf, MAX_BYTES);
				if (nread < 0)
					report_error_and_exit("Error reading from stdin in the client");

				bcopy(buf, server_buf, nread);
				if (encrypt_flag)
					mcrypt_generic(crypt_fd, server_buf, 1);
					// encrypt the data sendint to server
				if (write(client_socket_fd, server_buf, nread) < 0)
					report_error_and_exit("Error writing to socket in the client");

				if (log_flag)
				{
					char prefix[] = "SENT "; 
					if (write(log_fd, prefix, 5) < 0) 
						report_error_and_exit("Error writing to log file");
					char number[3];
					sprintf(number, "%d", nread);
					write(log_fd, number, strlen(number));
					char bytes[] = " bytes: ";
					if (write(log_fd, bytes, 8) < 0)
						report_error_and_exit("Error writing to log file");
					if (write(log_fd, server_buf, nread) < 0)
						report_error_and_exit("Error writing to log file");
					if (write(log_fd, "\n", 1) < 0)
						report_error_and_exit("Error writing to log file");
				}

				int i;
				for (i = 0; i < nread; ++i)
				{
					char ch = buf[i];
					if (ch == '\r' || ch == '\n')
					{	
						if(write(STDOUT_FILENO, "\r\n", 2) < 0)
							report_error_and_exit("Error writing to stdout in the client");
					}
					else if (ch == 0x04)  // input ^D
					{
						if (write(STDOUT_FILENO, "^D", 2) < 0)
							report_error_and_exit("Error writing to stdout in the client");
					}

					else if (ch == 0x03)  // input ^C
					{
						if (write(STDOUT_FILENO, "^C", 2) < 0)
							report_error_and_exit("Error writing to stdout in the client");
					}
					else
					{
						if (write(STDOUT_FILENO, &ch, 1) < 0)
							report_error_and_exit("Error writing to stdout in the client");
					}
				}
			}

			if (input_fds[1].revents & POLLIN) // pending input from server socket
			{
				nread = read(client_socket_fd, server_buf, nread);
				if (nread == 0) // the socket to the server is closed if we cannot actually read any byte 
					exit(0);
				if (nread < 0)
					report_error_and_exit("Error reading from socket in the client");
				bcopy(server_buf, buf, nread);
				if (encrypt_flag)
					mdecrypt_generic(decrypt_fd, buf, nread);
					// decrypt input from server
				if (write(STDOUT_FILENO, buf, nread) < 0)
					report_error_and_exit("Error writing to stdout in the client");

				if (log_flag)
				{
					char prefix[] = "RECEIVED "; 
					if (write(log_fd, prefix, 9) < 0) 
						report_error_and_exit("Error writing to log file");
					char number[3];
					sprintf(number, "%d", nread);
					write(log_fd, number, strlen(number));
					char bytes[] = " bytes: ";
					if (write(log_fd, bytes, 8) < 0)
						report_error_and_exit("Error writing to log file");
					if (write(log_fd, server_buf, nread) < 0)
						report_error_and_exit("Error writing to log file");
					if (write(log_fd, "\n", 1) < 0)
						report_error_and_exit("Error writing to log file");
				}
			}

			if (input_fds[1].revents & (POLLHUP | POLLERR))
			{
				close(client_socket_fd);
				exit(0); // exit client if socket from the server hang up
			}
		}
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

	close(client_socket_fd);

	if (encrypt_flag)
	{
		free(iv);
		free(key);
		mcrypt_generic_deinit(crypt_fd);
  		mcrypt_module_close(crypt_fd);
  		mcrypt_generic_deinit (decrypt_fd);
  		mcrypt_module_close(decrypt_fd);
	}
}
