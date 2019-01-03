
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <mraa.h>
#include <math.h>
#include <poll.h>


static volatile int run_flag = 1;

// temperature sensor parameters
#define TMP_SENSOR_PIN 1
// button parameters
#define BUTTON_PIN 60

mraa_aio_context tmp_sensor;
mraa_gpio_context button;


// optional arguments
int opt_sample, opt_log, period;
char opt_scale;
char* log_filename;
int log_fd;


// thermioster parameters
const int beta = 4275;
const int R_0 = 100000;
const float T_0 = 298.15;

// a helper function that reports an error message and exit 1
void report_error_and_exit(const char* msg);

// process a str that containes keyboard command terminated by zero byte
void parse_command(const char* arg);

// sample the temperature sensor once
void sample_tmp_sensor(const time_t raw_time);

void close_mraa();
// dellocate mraa contexts

// a helper function that reports an error message and exit 1
void report_error_and_exit(const char* msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}


// handle the off signal when button is pushed or OFF command is sent
void off_interrupt_handler()
{
	run_flag = 0;
	// when the button is pressed, prepare to exit
	struct timeval now;
	struct tm* time_info;

	if (gettimeofday(&now, NULL) < 0)
		report_error_and_exit("Error getting current time");
	time_info = localtime(&now.tv_sec);

	const int MAXBYTES = 25;
	char report[MAXBYTES];
	sprintf(report, "%02d:%02d:%02d SHUTDOWN\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

	size_t nwrite = strlen(report);
	if (write(STDOUT_FILENO, report, nwrite) < 0)
		report_error_and_exit("Failed to write report to stdout");
	if (opt_log)
	{
		if (write(log_fd, report, nwrite) < 0)
			report_error_and_exit("Failed to write report to stdout");
	}
	// report the final sample
	exit(0);
}

void close_mraa()
{
	// close the contexts
	mraa_aio_close(tmp_sensor);
	mraa_gpio_close(button);
}

int main(int argc, char* argv[])
{
	opt_sample = 1;
	opt_log = 0;
	opt_scale = 'F';
	period = 1;

	char* usage = "Correct usage: lab4b [--period][--scale][--log]\n";

	// get the optional arguments
	static const struct option long_options[] = 
	{
		{"period", required_argument, NULL, 'p'},
		{"scale", required_argument, NULL, 's'},
		{"log", required_argument, NULL, 'l'},
		{0, 0, 0, 0}
	};

	int c;
	while (1)
	{
		int option_index = 0;  // store the index of the next option we find
		c = getopt_long(argc, argv, "", long_options, &option_index); // get the next option

		if (c == -1)
			break;

		switch(c)
		{
			case 'p':      
				period = atoi(optarg);
				break;  // for --period flag
			case 'l':
				opt_log = 1;
				log_filename = optarg;
				break;  // for --log flag
			case 's':
				if (optarg[0] == 'F' || optarg[0] == 'C')
				{
					opt_scale = optarg[0];
					break;
				}  // for --scale flag
			default:    //case '?':
				fprintf(stderr, "%s\n", usage);
				exit(1);  // for unrecognized argument
		}

	}

	if (opt_log)
	{
		log_fd = creat(log_filename, 0666);
		if (log_fd < 0)
			report_error_and_exit("Error opening the log file");
	}


	tmp_sensor = mraa_aio_init(TMP_SENSOR_PIN);
	if (tmp_sensor == NULL)
	{
		fprintf(stderr, "Failed to initialize the temperature sensor\n");
		// mraa_deinit();
		exit(1);
	}

	button = mraa_gpio_init(BUTTON_PIN);
	if (button == NULL)
	{
		fprintf(stderr, "Failed to initialize the button\n");
		// mraa_deinit();
		exit(1);
	}

	// close the mraa contexts before exit
	atexit(close_mraa);

	if (mraa_gpio_dir(button, MRAA_GPIO_IN) != MRAA_SUCCESS)
		report_error_and_exit("Failed to set the input direction of button");

	if (mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &off_interrupt_handler, NULL) != MRAA_SUCCESS)
		report_error_and_exit("Failed to link button interrupt to handler");

	struct timeval time_now, time_next_read;
	time_next_read.tv_sec = 0;
	// time_next_read.tv_usec = 0;

	struct pollfd input_fds[1];
	input_fds[0].fd = STDIN_FILENO;
	input_fds[0].events = POLLIN;
	int timeout = 100;
	int poll_ret;
	// polling for stdin commands

	const int MAXBYTES = 20;
	char buffer[MAXBYTES];
	char argument[MAXBYTES];
	int i, nread;
	int pos = 0;

	while (run_flag)
	{
		if (gettimeofday(&time_now, NULL) < 0)
			report_error_and_exit("Failed to get current time");

		if (time_now.tv_sec >= time_next_read.tv_sec)
		{	
			if (opt_sample)
			{	
				sample_tmp_sensor(time_now.tv_sec);
				// sample the temperature sensor once
				time_next_read = time_now;
				time_next_read.tv_sec += period;
				// update the due for next read
			}
		}

		poll_ret = poll(input_fds, 1, timeout);
		if (poll_ret < 0)
			report_error_and_exit("Error polling for stdin channel");
		else if (poll_ret > 0)
		{
			if (input_fds[0].revents & POLLIN) 
				// find pending commands from stdin 
			{
				nread = read(STDIN_FILENO, buffer, MAXBYTES);
				if (nread < 0)
					report_error_and_exit("Error reading from stdin");
				for (i = 0; i < nread; ++i)
				{
					char ch = buffer[i];
					if (ch == '\n')
					{
						argument[pos] = '\0';
						parse_command(argument);
						pos = 0; // process the current command and clear the argument buffer
					}
					else
					{
						argument[pos] = ch;
						pos++; 
						// if only partial line is read, start with pos in argument for the next read
					}
				}
				
			}
                                          
			if (input_fds[0].revents & POLLERR)
				report_error_and_exit("Error reading from stdin");
			
		} // finish processing read from keyboard




	}

	exit(0);
}

// sample the temperature sensor once every period secs
void sample_tmp_sensor(const time_t raw_time)
{
	int tmp_value = mraa_aio_read(tmp_sensor);
	float R = 1023.0 / tmp_value - 1.0;
	R = R_0 * R;

	float tmp_K = 1.0 / (logf(R / R_0) / beta +  1.0 / T_0);
	float tmp_C = tmp_K - 273.15;
	float tmp_F = tmp_C * (9.0 / 5.0) + 32;
	float temperature;

	if (opt_scale == 'F')
		temperature = tmp_F;
	else if (opt_scale == 'C')
		temperature = tmp_C;

	struct tm* time_info;
	time_info = localtime(&raw_time);
	if (time_info == NULL)
		report_error_and_exit("Failed to get local time");

	const int MAXBYTES = 20;
	char report[MAXBYTES];
	sprintf(report, "%02d:%02d:%02d %.1f\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec, temperature);

	size_t nwrite = strlen(report);
	if (write(STDOUT_FILENO, report, nwrite) < 0)
		report_error_and_exit("Failed to write report to stdout");
	if (opt_log)
	{
		if (write(log_fd, report, nwrite) < 0)
			report_error_and_exit("Failed to write report to stdout");
	}
	// report the current sample

}

// process a c string that contains a keyboard command
void parse_command(const char* arg)
{
	if (opt_log)
	{
		size_t nwrite = strlen(arg);
		if (write(log_fd, arg, nwrite) < 0)
			report_error_and_exit("Error writing commands to log file");
		if (write(log_fd, "\n", 1) < 0)
			report_error_and_exit("Error writing commands to log file");
	} // first echo command to log file

	if (strcmp(arg, "STOP") == 0)
		opt_sample = 0;
		// stop reading the temperature sensor
	else if (strcmp(arg, "START") == 0)
		opt_sample = 1;
		// continue to read the temperature sensor
	else if (strcmp(arg, "OFF") == 0)
		off_interrupt_handler();
		// shut down the program
	else if (strncmp(arg, "SCALE=F", strlen("SCALE=F")) == 0)
		opt_scale = 'F';
	else if (strncmp(arg, "SCALE=C", strlen("SCALE=C")) == 0)
		opt_scale = 'C';
	else if (strncmp(arg, "PERIOD=", strlen("PERIOD=")) == 0)
		period = atoi(arg + 7);
	else if (strncmp(arg, "LOG", strlen("LOG")) == 0)
		{}
	/*
	else
		report_error_and_exit("Unrecognized keyboard command");
	*/
	

}
