%{
//#define yylex yylex
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "command.h"


void yyerror(const char * s);
int yylex();
%}

%token	<string_val> WORD SUBSHELLCOMMAND
%token 	NOTOKEN NEWLINE GREAT GREATAMPERSAND GREATGREAT GREATGREATAMPERSAND LESS AMPERSAND PIPE
%union	{
		char   *string_val;
	}
%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
        ;
	
simple_command:	
	pipe_list io_modifier_list background_opt NEWLINE {
		
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args
	;


command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {

	       	expandWildcardsIfNecessary($1);
		
	}
	| SUBSHELLCOMMAND {

		
		int count = 0;

		int fdpipe[2];
		pipe(fdpipe);

		int pid = fork();
		
		if(pid == 0) {

			Command::_currentSimpleCommand = new SimpleCommand();

			char * whole = $1;

			char * arg = (char *) malloc (sizeof(char) * 300);

			for (int i = 0; i < strlen(whole); i ++) {

				if (whole[i] == ' ') {
				
					arg[count] = '\0';

					Command::_currentSimpleCommand->insertArgument(arg);

					count = 0;
				}

				else {

					arg[count] = whole[i];

					count ++;
				}
			}

			arg[count] = '\0';

			Command::_currentSimpleCommand->insertArgument(arg);

			dup2(fdpipe[1], 1);
			dup2(fdpipe[0], 0);

			Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
          		Command::_currentCommand.execute();

			_exit(0);

		}
		
		count = 0;

		char * buffer = (char *)malloc(sizeof(char) * 1000);
		
		count = read(fdpipe[0], buffer, 1000);

		buffer[count] = '\0';

		int previous = 0;
		
		for (int i = 0; i < strlen(buffer); i ++) {
	

			if ((buffer[i] == ' ') || (i == strlen(buffer) - 1) || (buffer[i] == '\n')) {
	
				char * temp_arg = (char *)malloc(sizeof(char) * 1000);

				strncpy(temp_arg, buffer + previous, i - previous);

				temp_arg[i-previous] = '\0';

				Command::_currentSimpleCommand->insertArgument(strdup(temp_arg));

				free(temp_arg);

				previous = i + 1;
			}
		}

 	}
	;

command_word:
	WORD {
              	      
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

io_modifier_list:
	io_modifier_list io_modifier_opt
	| /*empty*/
	;

background_opt:
	AMPERSAND {
	
		Command::_currentCommand._background=1;
	}
	| /*empty*/
	;

io_modifier_opt:
	GREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;

		Command::_currentCommand.outFile_count = Command::_currentCommand.outFile_count + 1;

	}
	| GREATAMPERSAND WORD {

		//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;

		//printf("   Yacc: insert err \"%s\"\n", $2);
		Command::_currentCommand._errFile = strdup($2);

		Command::_currentCommand.outFile_count = Command::_currentCommand.outFile_count + 1;


	}
	| GREATGREAT WORD {

		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append=1;

		Command::_currentCommand.outFile_count = Command::_currentCommand.outFile_count + 1;


	}
	| GREATGREATAMPERSAND WORD {
		
		Command::_currentCommand._outFile = $2;

		Command::_currentCommand._errFile = strdup($2);
		
		Command::_currentCommand._append=1;

		Command::_currentCommand.outFile_count = Command::_currentCommand.outFile_count + 1;
	
	}
	| LESS WORD {

		//printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;

		Command::_currentCommand.inputFile_count = Command::_currentCommand.inputFile_count + 1;

	}
	|
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
