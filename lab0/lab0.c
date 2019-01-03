
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>

void make_seg_fault(char* c)
{
	c[0] = 'a';
}

void seg_fault_handler(int signum)
{
	if (signum == SIGSEGV)
	{
		fprintf(stderr, "Caught segmentation fault\n");
		exit(4);
	}
}

int main(int argc, char* argv[])
{
	int input_flag , output_flag, seg_flag, catch_flag;
	int option_index;

    // initially no arguments are set
	input_flag = 0;
	output_flag = 0;
	seg_flag = 0;
	catch_flag = 0;

	char* input_filename;
	char* output_filename;

	// support four types of long optional arguments
	static struct option long_options[] = 
	{
		{"input", required_argument, NULL, 'i'},
		{"output", required_argument, NULL, 'o'},
		{"segfault", no_argument, NULL, 's'},
		{"catch", no_argument, NULL, 'c'},
		{0, 0, 0, 0}
	};

	char c;
	while (1)
	{
		option_index = 0;  // store the index of the next option we find
		c = getopt_long(argc, argv, "", long_options, &option_index); // get the next option

		if (c == -1)
			break;

		switch(c)
		{
			case 'i':      
				input_flag = 1;
				input_filename = optarg;
				break;  // for --input flag
			case 'o':
				output_flag = 1;
				output_filename = optarg;
				break;  // for --output flag
			case 's':
				seg_flag = 1;
				break;  // for --seg flag
			case 'c':
				catch_flag = 1;
				break;  // for --catch flag
			default:    //case '?':
				fprintf(stderr, "Correct usage: lab0 [--input input_file_name] [--output output_file_name] [--segfault] [--catch]\n");
				exit(1);  // for unrecognized argument
			
		}

	}

	/*
	if (optind < argc)
	{
		fprintf(stderr, "additional arguments in argv: ");
		while (optind < argc)
		{
			fprintf(stderr, "%s ", argv[optind]);
			optind++;
		}
		fprintf(stderr, "\n");
		exit(1);
	}
	*/

	if (input_flag)  // if --input is set
	{
		int ifd = open(input_filename, O_RDONLY);
		if (ifd < 0) 
		// unable to open the input file
		{
			fprintf(stderr, "Error opening the --input file %s: %s\n", input_filename, strerror(errno));
			exit(2);
		}
		// redirect the input
		close(STDIN_FILENO);
		dup(ifd);
		close(ifd);
	}

	if (output_flag)  // if --output is set
	{
		int ofd = creat(output_filename, 0666);
		if (ofd < 0)   // unable to create the output file
		{
		fprintf(stderr, "Error creating the --output file %s: %s\n", output_filename, strerror(errno));
		exit(3);
		}
		// redirect the output
		close(STDOUT_FILENO);
		dup(ofd);
		close(ofd);
	}

	if (catch_flag)
	{
		signal(SIGSEGV, seg_fault_handler);
	}

	char* problem = NULL;
	if (seg_flag)
	{		
		make_seg_fault(problem);
	}

	// read from stdin and write to stdout
	const int COUNT = 32;
	char* buffer = (char*) malloc(COUNT);
	int nread, nwrite;

	nread = read(STDIN_FILENO, buffer, COUNT);
	while (nread > 0)
	{
		nwrite = write(STDOUT_FILENO, buffer, nread);
		if (nwrite < 0)
		{
			fprintf(stderr, "Error writing to the output file: %s\n", strerror(errno));
			exit(3);
		}
		nread = read(STDIN_FILENO, buffer, COUNT);
	}

	if (nread < 0)
	{
		fprintf(stderr, "Errror reading from the input file: %s\n", strerror(errno));
		exit(2);
	}

	free(buffer);
	// reached end of file char of the input file
	exit(0);

}

