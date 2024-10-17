#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"
#define DEFAULT RESET

#define CLEAR "\x1b[2J"

/* Define colors for questions/answers/highlights */
#define QCOL  BLUE
#define ACOL  DEFAULT
#define HLCOL GREEN

struct termios new, old;
FILE *file;

void usage(const char *name, FILE *f)
{
	fprintf(f, "usage: %s [-q <question character>] [-h <highlight character>] <file>\n", name);
}

void restore_term(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
}

void close_file(void)
{
	fclose(file);
}

void print_answer(const char *line, char hlc)
{
	printf(ACOL);
	int hl = 0;
	for (; *line; line++)
	{
		if (*line == hlc)
		{
			if (hl)
			{
				printf(ACOL);
				hl = 0;
			}
			else
			{
				printf(HLCOL);
				hl = 1;
			}
			continue;
		}
		fputc(*line, stdout);
	}
	printf(RESET "\r");
}

void get_input(void)
{
	int done = 0;
	while (!done)
	{
		switch (fgetc(stdin))
		{
		case 3:
		case 4:
		case 'q':
			exit(0);
		case ' ':
		case '\r':
			done = 1;
			break;
		default:
			break;
		}
	}
}

int main(int argc, char **argv)
{
	const char *fname;
	char qc = '*';
	char hlc = '~';

	if (argc < 2)
	{
		usage(argv[0], stderr);
		exit(1);
	}

	/* Parse command line args */
	for (argv++; *argv; argv++)
	{
		char *arg = *argv;
		if (*arg == '-')
		{
			arg++;
			switch (*arg)
			{
			case 'q':
				arg++;
				if (strlen(arg) == 0)
				{
					argv++;
					arg = *argv;
				}
				if (!arg || strlen(arg) > 1)
				{
					usage(argv[0], stderr);
					exit(1);
				}
				qc = *arg;
				break;
			case 'h':
				arg++;
				if (strlen(arg) == 0)
				{
					argv++;
					arg = *argv;
				}
				if (!arg || strlen(arg) > 1)
				{
					usage(argv[0], stderr);
					exit(1);
				}
				hlc = *arg;
				break;
			default:
				usage(argv[0], stderr);
				exit(1);
			}
			continue;
		}
		fname = strdup(arg);
	}

	if (!fname)
	{
		usage(argv[0], stderr);
		exit(1);
	}

	/* Open file for reading */
	file = fopen(fname, "r");
	if (!file)
	{
		perror("fopen");
		exit(1);
	}
	atexit(close_file);

	/* Set terminal raw mode */
	tcgetattr(STDIN_FILENO, &old);
	memcpy(&new, &old, sizeof(struct termios));
	cfmakeraw(&new);
	tcsetattr(STDIN_FILENO, TCSANOW, &new);
	atexit(restore_term);

	char *line = NULL;
	size_t size = 0;
	int is_q = 0;
	while (getline(&line, &size, file) > 0)
	{
		if (line[0] == qc)
		{
			is_q = 1;
			line++;
			while (*line == ' ')
				line++;
			printf(QCOL "%s\r" RESET, line);
			continue;
		}

		if (is_q)
		{
			get_input();
		}

		print_answer(line, hlc);
		is_q = 0;
	}
}
