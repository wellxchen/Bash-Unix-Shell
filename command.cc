
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <regex.h>
#include <assert.h>
#include <algorithm>
#include "command.h"
#include <pwd.h>

#define MAXFILENAME 1024

extern char ** environ;

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

int cmpfunc (const void *name1, const void *name2)
{
	const char *name1_ = *(const char **)name1;
	const char *name2_ = *(const char **)name2;
	return strcmp(name1_, name2_);
}

/*perform expansion for wildcards that needed to be expanded*/

void expandWildcards(char * prefix, char *suffix, char **array, int * nEntries, int * maxEntries) {

	if (suffix[0]== 0) {

		int added = 1;

		for (int i = strlen(prefix) - 2; i >= 0; i --) {

			if ((prefix[i] == '/') && (prefix[i + 1] == '.')) {

				added --;
				break;
			}
		}

		if (added > 0) {
			array[*nEntries] = strdup(prefix);
			(*nEntries) ++;
		}

		return;
	}

	char * s = strchr(suffix, '/');
	char component[MAXFILENAME];
	if (s!=NULL){
		strncpy(component,suffix, s-suffix);
		suffix = s + 1;
	}
	else {
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}

	char newPrefix[MAXFILENAME];

	if ((!strchr(component, '*')) && (!strchr(component, '?'))) {

		if (strcmp(prefix, "/") == 0) {

			sprintf(newPrefix,"/%s", component);

		}

		else {
			sprintf(newPrefix,"%s/%s",prefix, component);

		}

		expandWildcards(newPrefix, suffix, array, nEntries, maxEntries);
		return;
	}

	char * reg = (char *)malloc(2 * strlen(component) + 10);
	char *a = component;
	char * r = reg;
	*r = '^';
	r++;

	while (*a) {
		if (*a == '*') { *r='.'; r++; *r='*'; r++; }
		else if (*a == '?') { *r='.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
		else { *r=*a; r++;}
		a++;
	}
	*r='$'; r++; *r='\0';

	regex_t re;
	if (regcomp(&re, reg, REG_EXTENDED|REG_NOSUB) != 0){
		perror("regular expression compile.\n");
		return;
	}

	struct dirent * ent;
	char * dir = prefix; /**/

	DIR * d=opendir(dir);

	if (d==NULL) return;

	while ((ent = readdir(d))!= NULL) {

		regmatch_t match;

		if (regexec(&re, ent -> d_name, 1, &match,0) == 0) {

			if (strcmp(prefix, "/") == 0) {

				sprintf(newPrefix,"/%s", ent->d_name);

			}

			else {
				sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
			}

			expandWildcards(newPrefix,suffix, array, nEntries, maxEntries);
		}
	}

	closedir(d);
}

/*check if it is necessary to expand wildcard*/

void expandWildcardsIfNecessary (char * arg) {

	if ((strchr(arg, '*') || strchr(arg, '?'))) {

		int nEntries = 0;

		int maxEntries = 10000;

		char ** array = (char **)malloc(maxEntries * sizeof (char *));


		if (strchr(arg, '/') != NULL) {

			char * prefix = NULL;

			char * suffix = NULL;

			for (int i = 0; i < strlen(arg); i ++) {

				if ((arg[i] == '*') || (arg[i] == '?')) {

					for (int j = i; j >= 0; j --) {

						if (arg[j] == '/') {

							if (j != 0) {

								prefix = (char *)malloc(sizeof(char) * j);

								strncpy(prefix, arg, j);

								prefix[j] = '\0';
							}

							else {
								prefix = (char *)malloc(sizeof(char) * 2);
								prefix[0] = '/';
								prefix[1] = '\0';
							}

							suffix = (char *)malloc(sizeof(char) * (strlen(arg) - j));

							strncpy(suffix, &arg[j + 1], strlen(arg) - j);

							suffix[strlen(arg) - j] = '\0';

							break;
						}
					}
				}
				if (suffix != NULL) {

					break;
				}
			}

			expandWildcards(prefix, suffix, array, &nEntries, &maxEntries);
		}

		else {

			char * reg = (char *)malloc(2 * strlen(arg) + 10);
			char *a = arg;
			char * r = reg;
			*r = '^';
			r++;

			while (*a) {
				if (*a == '*') { *r='.'; r++; *r='*'; r++; }
				else if (*a == '?') { *r='.'; r++;}
				else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
				else { *r=*a; r++;}
				a++;
			}
			*r='$'; r++; *r='\0';

			regex_t re;
			if (regcomp(&re, reg, REG_EXTENDED|REG_NOSUB) != 0){
				perror("regular expression compile.\n");
				return;
			}

			struct dirent * ent;

			DIR * dir = opendir(".");

			while ((ent = readdir(dir))!= NULL)  {

				regmatch_t match;

				if (regexec(&re, ent -> d_name, 1, &match,0) == 0) {


					if (nEntries == maxEntries) {

						maxEntries *= 2;
						array = (char **)realloc(array, maxEntries*sizeof(char *));
						assert(array!=NULL);
					}

					if (ent -> d_name[0] == '.') {

						if (arg[0] == '.') {

							array[nEntries] = strdup(ent->d_name);
							nEntries ++;
						}
					}

					else {

						array[nEntries] = strdup(ent->d_name);
						nEntries ++;
					}
				}
			}

			closedir(dir);
			regfree(&re);
		}

		qsort(array, nEntries, sizeof(char *), cmpfunc);

		for (int i = 0; i < nEntries; i ++) {

			Command::_currentSimpleCommand->insertArgument(strdup(array[i]));

		}
		free(array);
	}

	else {
		Command::_currentSimpleCommand->insertArgument(arg);
	}
}

	void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				_numberOfAvailableArguments * sizeof( char * ) );
	}

	/*environ expansion*/

	char * complete_arg = (char *)malloc(sizeof(char) * 1024);

	complete_arg [0] = '\0';

	char NAME [1024];

	int actual_length = 0;

	for (int i = 0; i < strlen(argument); i ++) {

		if ((i <= (strlen(argument) - 4)) 
				&& (argument[i] == '$') && 
				(argument[i + 1] == '{')) {

			int j = i + 2;

			while ((j < strlen(argument)) && (argument[j] != '}')) {

				j ++;
			}

			strncpy (NAME, &argument[i + 2], (j - i - 2));
			NAME[j - i - 2] = '\0';

			strcat(complete_arg, getenv(NAME));

			actual_length += (strlen(getenv(NAME)));
			i = j;
		}

		else {

			complete_arg[actual_length] = argument[i];
			actual_length ++;
		}

		complete_arg[actual_length] = '\0';
	}

	/*tilde expansion*/

	if (complete_arg[0] == '~') {


		struct passwd *ps;

		char * user = (char *)malloc(sizeof(char) * 1024);

		char * userdir;

		int breakpoint = 0;

		char * temp = (char *)malloc(sizeof(char) * 1024);

		if (actual_length > 1) {

			if (strchr(complete_arg, '/')) {

				if (complete_arg[1] != '/') {

					int iii = 1;

					while(iii < actual_length) {

						if (complete_arg[iii] == '/') {

							breakpoint = iii;

							break;
						}
						else {
							user[iii - 1] = complete_arg[iii]; 
						}

						iii ++;
					}

					user[iii - 1] = '\0';

					ps = getpwnam(user);

					userdir = strdup(ps ->  pw_dir);
				}

				else {

					breakpoint = 1;

					userdir = strdup(getenv("HOME"));
				}

				sprintf(temp, "%s%s", userdir, complete_arg + breakpoint);
				temp[strlen(userdir) + actual_length - breakpoint] = '\0';
			}

			else {


				for (int o = 1; o < actual_length; o ++) {

					user[o - 1] = complete_arg[o];

					if (o == actual_length - 1) {

						user[o] = '\0';
					}
				}


				ps = getpwnam(user);
				userdir = strdup(ps ->  pw_dir);

				sprintf(temp, "%s", userdir);
				temp[strlen(userdir)] = '\0';
			}
		}

		else {

			userdir = strdup(getenv("HOME"));

			sprintf(temp, "%s", userdir);
			temp[strlen(userdir)] = '\0';
		}	


		for (int k = 0; k < strlen(temp); k ++) {

			complete_arg[k] = temp[k];
			if (k == (strlen(temp) -1)) {

				complete_arg[k + 1] = '\0';
			}
		}

		free(user);
		free(temp);
	}

	/*add argument*/

	_arguments[ _numberOfArguments ] = complete_arg;

	_arguments[ _numberOfArguments + 1] = NULL;

	_numberOfArguments++;
}

Command::Command()
{
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

	void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
				_numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}

	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

	void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}

		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}
	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	inputFile_count = 0;
	outFile_count = 0;
}

	void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background    Append\n" );
	printf( "  ------------ ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
			_inputFile?_inputFile:"default", _errFile?_errFile:"default",
			_background?"YES":"NO", _append?"YES":"NO");
	printf( "\n\n" );

}

	void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	if (inputFile_count > 1) {

		printf("Ambiguous input redirect");
		prompt();
		return;
	}

	if (outFile_count > 1) {

		printf("Ambiguous output redirect");
		prompt();
		return;
	}

	/*redirect input, output, and err*/

	int defaultin = dup( 0 );
	int defaultout = dup( 1 );
	int defaulterr = dup( 2 );

	int in;
	int out;
	int err;

	int pid;

	if (_inputFile){

		in = open (_inputFile, O_RDONLY);
	}
	else {
		in = dup(defaultin);
	}

	if (_errFile) {
		if (_append) {

			err = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
		}
		else {

			err = open (_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		}
	} 
	else {
		err = dup(defaulterr);
	}

	dup2(err, 2);
	close(err);

	/*if exit is entered, exit shell*/

	if (!strcmp(*(_simpleCommands[0] -> _arguments), "exit")) {

		fprintf(stderr, "Good bye!!\n");
		prompt();
		exit(1);
	}

	/*iterate all commands*/

	for (int i = 0; i < _numberOfSimpleCommands; i ++) {


		dup2(in, 0);
		close(in);

		if (i == _numberOfSimpleCommands - 1) {


			if (_outFile) {
				if (_append) {

					out = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
				}
				else {

					out = open (_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				}
			}

			else {

				out=dup(defaultout);
			}
		}

		else {

			int fdpipe[2]; 

			if ( pipe(fdpipe) == -1) {
				perror( "pipe");
				exit( 2 );
			}

			out = fdpipe[1];
			in = fdpipe[0];
		}

		dup2(out, 1);
		close(out);

		/*cd*/

		if (!strcmp( _simpleCommands[i]->_arguments[0], "cd" )) {

			if (_simpleCommands[i] -> _numberOfArguments == 1) {

				chdir(getenv("HOME"));
			}

			else {

				if (chdir(_simpleCommands[i]->_arguments[1]) == -1) {
					fprintf(stderr, "No such file or directory\n");
				}
			}
		}


		/*setenv*/

		if ( !strcmp( _simpleCommands[i]->_arguments[0], "setenv" ) )
		{
			setenv( _simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);
		}

		/*unsetenv*/

		else if (!strcmp( _simpleCommands[i]->_arguments[0], "unsetenv" )) {

			int iterate = 0;
			while (environ[iterate] != NULL) {

				if (strstr(environ[iterate], _simpleCommands[i]->_arguments[1]) != NULL) {


					free(environ[iterate]);
					environ[iterate] = NULL;

					break;
				}

				iterate ++;
			}
		}


		/*fork child process and execute command*/

		else {

			pid = fork();

			if ( pid == -1 ) {
				perror( "fork\n");
				exit( 2 );
			}

			if ( pid == 0 ) {

				if (!strcmp( _simpleCommands[i]->_arguments[0], "printenv" )) {

					printenv();
				}

				execvp(*(_simpleCommands[i] -> _arguments), _simpleCommands[i] -> _arguments);
				exit( 2 );
			}
		}
	}

	dup2 (defaultin, 0);
	dup2 (defaultout, 1);
	dup2 (defaulterr, 2);

	close (defaultin);
	close (defaultout);
	close (defaulterr);

	if (!_background) {

		waitpid(pid, NULL,0);
	}


	/**/

	clear();


	prompt();

}


void 
Command::printenv() {

	int i = 0;

	while (*(environ + i)!= NULL) {

		printf("%s\n", *(environ + i));
		i ++;
	}
}

// Shell implementation

	void
Command::prompt()
{
	if (isatty(0) == 1) {
		printf("myshell>");
	}

	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

extern "C" void	disp (int signo) 
{
	if(signo == SIGINT) {
		printf("\n");

	} else if(signo == SIGCHLD) {
		while (waitpid(-1, NULL, WNOHANG) > 0);

	}
}

main()
{

	signal(SIGINT, disp);
	signal(SIGCHLD, disp);
	Command::_currentCommand.prompt();
	yyparse();
}

