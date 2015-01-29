

%{

#include <string.h>
#include "y.tab.h"
#include <algorithm>
#include <unistd.h>


////////////  Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.

  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

">&"	{
		return GREATAMPERSAND;
	}

">>"	{
		return GREATGREAT;
	}

">>&"	{
		return GREATGREATAMPERSAND;
	}

"<"	{
		return LESS;
	}

"&"	{
		return AMPERSAND;
	}

"|"	{
		return PIPE;
	}


`[^\n]*` {

		yylval.string_val = (char *)calloc(strlen(yytext) - 1, sizeof(char));

		strncpy(yylval.string_val, yytext + 1, strlen(yytext-2));

		yylval.string_val[strlen(yytext)-2] = '\0';

		return SUBSHELLCOMMAND;

	}


[^ \t\n]*[\\][^ \t\n]* {

	
	yylval.string_val = (char *)calloc(strlen(yytext)-1, sizeof(char));

	int j = 0;
	
	for (int i = 0; i < strlen(yytext) - 1; i ++) {
		
		if (yytext[i] == '\\') {
		
			i ++;
			yylval.string_val[j] = yytext[i];
			j ++;
		}

		else {
			yylval.string_val[j] = yytext[i];
			j ++;

		}
	}

	yylval.string_val[j] = '\0';
	
	if ((yylval.string_val[0] == '`') && (yylval.string_val[j - 1] == '`')) {
		
		char * temp = (char *)malloc(sizeof(char) * j);

		strcpy(temp, yylval.string_val);

		temp[j] = '\0';

		strncpy (yylval.string_val, temp + 1, strlen(temp) - 2);

		yylval.string_val[strlen(temp) - 2] = '\0';
	
		free(temp);
		
		return SUBSHELLCOMMAND;
	}

	else {

		return WORD;
	}
}



\"[^\n\"]*\" {

	yylval.string_val = (char *)calloc(strlen(yytext)-1, sizeof(char));
	strncpy(yylval.string_val, yytext + 1, strlen(yytext-2));
 	yylval.string_val[strlen(yytext)-2] = '\0';
	return WORD;
}

[^ \t\n|&><][^ \t\n|&><]*  {
	/* Assume that file names have only alpha chars */
	yylval.string_val = strdup(yytext);
	return WORD;
}




