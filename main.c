#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>

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
char *pattern = NULL;
regex_t regex;

void usage(const char *name, FILE *f)
{
	fprintf(f, "Usage: %s [options...] <file>\n", name);
	fprintf(f, "Options:\n"
	           "\t-q <character> \tSpecify character to indicate start of a question line (default: '*')\n"
	           "\t-h <character> \tSpecify character that surrounds a highlighted word (default: '~')\n"
	           "\t-l <number>    \tSkip to a specific line\n"
		   "\t-r <pattern>   \tJump to first line matching a regular expression\n");
}

void restore_term(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	printf("\n");
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
	printf("Press <enter> to show answers, <r> to reload file, <q>/<C-c> to exit...");
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
			printf("\x1b[2K\r");
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
	int start = 0;
	char *name = *argv;

	if (argc < 2)
	{
		usage(name, stderr);
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
					usage(name, stderr);
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
					usage(name, stderr);
					exit(1);
				}
				hlc = *arg;
				break;
			case 'l':
				arg++;
				if (strlen(arg) == 0)
				{
					argv++;
					arg = *argv;
				}
				if (!arg)
				{
					usage(name, stderr);
					exit(1);
				}
				start = atoi(arg);
				start--;
				break;
			case 'r':
				arg++;
				if (strlen(arg) == 0)
				{
					argv++;
					arg = *argv;
				}
				if (!arg)
				{
					usage(name, stderr);
					exit(1);
				}
				pattern = strdup(arg);
				break;
			default:
				usage(name, stderr);
				exit(1);
			}
			continue;
		}
		fname = strdup(arg);
	}

	if (!fname)
	{
		usage(name, stderr);
		exit(1);
	}

	/* Compile regex */
	int err = 0;
	if (pattern && (err = regcomp(&regex, pattern, REG_ICASE | REG_NOSUB | REG_NEWLINE)))
	{
		char buf[512];
		regerror(err, &regex, buf, sizeof(buf));
		fprintf(stderr, "failed to compile regex: %s\n", buf);
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
		if (lineno < start)
			continue;

		if (pattern)
		{
			regmatch_t unused_regmatch;
			if (regexec(&regex, line, 1, &unused_regmatch, 0) == REG_NOMATCH)
				continue;
			pattern = NULL;
		}

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
