#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

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
char *fname = NULL;
int lineno = 0;

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

int get_input(void)
{
	int c;
	for (;;)
	{
		switch ((c = fgetc(stdin)))
		{
		case 3:
		case 4:
		case 'q':
			exit(0);
		case ' ':
		case '\r':
		case 'r':
			return c;
			break;
		default:
			break;
		}
	}
}

void reload_file(void)
{
	FILE *tmp;
	tmp = fopen(fname, "r");
	if (!tmp)
	{
		printf(RED "error reloading file: %s" RESET, strerror(errno));
		get_input();
		printf("\x1b[2K\r");
	}
	fclose(file);
	file = tmp;

	char *line = NULL;
	size_t size;
	for (int lineno_skip = 0; lineno_skip < lineno - 1; lineno_skip++)
	{
		if (getline(&line, &size, file) <= 0)
			exit(0);
	}
	free(line);

	printf(GREEN "file reloaded." RESET);
	get_input();
	printf("\x1b[2K\r");
}

int main(int argc, char **argv)
{
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
		lineno++;
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
			if (get_input() == 'r')
			{
				reload_file();
				continue;
			}
		}

		print_answer(line, hlc);
		is_q = 0;
	}
}
