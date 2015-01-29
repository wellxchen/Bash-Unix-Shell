/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <dirent.h>
#include <regex.h>


#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int cursor_pos = 0;
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
int history_length = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
   
  struct termios orig_attr;
  tcgetattr(0, &orig_attr);

  // Set terminal in raw mode
  tty_raw_mode();

  char ** history = (char **)malloc(sizeof (char *) * 1000);

      history[0] = (char *)malloc(sizeof(char) * 1000);


     	 FILE * in = fopen("history.txt", "r");

	 if (in != NULL) {
		
     		 char c = 0;

     		 int iterate = 0;

     		 while ((c = fgetc(in)) != EOF) {
      	
			if (c == '\n') {

				history[history_index][iterate] = '\0';
				iterate = 0;
				history_index ++;
				history_length ++;
				history[history_index] = (char *)malloc(sizeof(char) * 1000);

			}

			else {
				
				history[history_index][iterate] = c;
				iterate ++;
			}	
		}

		history[history_index][iterate] = '\0';	
		fclose(in);

	}

	
  line_length = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127 && ch != 8) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      // add char to buffer.
      line_buffer[line_length]=ch;
      line_length++;
      cursor_pos ++ ;	

    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);
      cursor_pos = 0;
      break;
    }

    else if (ch == 31) {

      char ch1; 
      read(0, &ch1, 1);
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }

    /*ctrl-A*/

    else if (ch == 1) {
    
	
    	while (cursor_pos >= 1) {
	
		ch = 8;
      		write(1,&ch,1);

		cursor_pos --;
	}
    }

    /*ctrl-E*/

    else if (ch == 5) {

    	while (cursor_pos < line_length) {
    
    		ch = line_buffer[cursor_pos];

		write(1, &ch, 1);

		cursor_pos ++;
    	}
    }


   /*ctrl-D*/
    else if (ch == 4) {


	if ((cursor_pos >= 0 ) && (cursor_pos < line_length)) {


    	if (cursor_pos == line_length - 1) {

		line_buffer[line_length - 1] = '\0';
    
		ch = ' ';
     		write(1,&ch,1);

		ch = 8;
     		write(1,&ch,1);
	}

	else {

		int j = cursor_pos;

	 	for (j = cursor_pos; j < line_length - 1; j ++) {
		
			ch = line_buffer[j + 1];
			write(1, &ch, 1);

			line_buffer [j] = line_buffer[j + 1]; 
		}

		line_buffer[line_length - 1] = '\0';

		ch = ' ';
     		write(1,&ch,1);


		j = line_length;

		for (j = line_length; j > cursor_pos; j --) {

			ch = 8;
     			write(1,&ch,1);
		}

	}

	line_length --;
	}
    }

   
    /*ctrl-H and backspace*/
    else if (ch == 127 || ch == 8)  {
      // <backspace> was typed. Remove previous character read.

	if (cursor_pos >= 1) {
      	// Go back one character
     	 ch = 8;
     	 write(1,&ch,1);
	 
	 if (cursor_pos == line_length) {

		line_buffer[line_length - 1] = '\0';
		
	 	ch=' ';
		write(1,&ch,1);

      		ch = 8;
     		write(1,&ch,1);		
	 }

     	 else {

		int j = cursor_pos;

	 	for (j = cursor_pos; j < line_length; j ++) {
		
			ch = line_buffer[j];
			write(1, &ch, 1);

			line_buffer [j - 1] = line_buffer[j]; 
		}

		ch=' ';
		write(1,&ch,1);

		line_buffer[j] = '\0';
		
		j = line_length;

		for (j = line_length; j >= cursor_pos; j --) {

			ch = 8;
     			write(1,&ch,1);
		}
	 }
     	 
     	 // Remove one character from buffer
	cursor_pos --;

      	line_length--;
      	
      }
    }

     /*tab*/

    else if (ch == 9) {

	line_buffer[line_length]=10;
	 line_length++;
 	 line_buffer[line_length]=0;

	 DIR * dir;

	 char * prefix = (char *)malloc(sizeof(char) * 500);
	 prefix[0] = '\0';

	 char * suffix = (char *)malloc(sizeof(char) * 500);
	 suffix[0] = '\0';
	
	if (strchr(line_buffer, '/')) {

		int findpath = line_length - 2;

		for (findpath = line_length - 2; findpath >= 0; findpath --) {

			/* path/file */

	 		if (line_buffer[findpath] == '/') {

				strncpy(prefix, line_buffer, findpath - 1);
				prefix[findpath - 1] = '\0';
				
				strncpy(suffix, line_buffer + findpath + 1, line_length - 1 - findpath);
				suffix[line_length - 1 - findpath] = '\0';

				break;
			}


		}

		dir = opendir(prefix);
	}

	else {
	
		dir = opendir(".");
		strncpy(suffix, line_buffer, line_length);
		suffix[line_length] = '\0';

	}

	 struct dirent * ent;	
	
	regex_t re;
	
	regcomp(&re, suffix, REG_EXTENDED|REG_NOSUB);

	while ((ent = readdir(dir)) != NULL) {

		regmatch_t match;

		if (regexec(&re, ent -> d_name, 1, &match, 0) == 0) {

			line_buffer[0] = '\0';
		
			if (prefix[0] != '\0') {

				sprintf(line_buffer, "%s/%s", prefix, ent -> d_name);
				line_buffer[strlen(prefix) + strlen(ent -> d_name) + 1] = '\0';
			}

			else {
			
				strcpy(line_buffer, ent -> d_name);
				line_buffer[strlen(ent -> d_name)] = '\0';
			}

	
			int i = 0;
			for (i =0; i < line_length; i++) {
	 		 	ch = 8;
	  			write(1,&ch,1);
			}


			for (i =0; i < line_length; i++) {
	 			 ch = ' ';
	 			 write(1,&ch,1);
			}

			for (i =0; i < line_length; i++) {
	 			 ch = 8;
	  			write(1,&ch,1);
			}	


			line_length = strlen(line_buffer);
			cursor_pos = line_length;
			write(1, line_buffer, line_length);
			break;
		}
	}
	closedir(dir);
    }


    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

     
      /*Up arrow*/

      if (ch1==91 && ch2==65) {

      	if (history_index != 0) {
	// Erase old line
	// Print backspaces
		int i = 0;
		for (i =0; i < line_length; i++) {
	  	ch = 8;
	 	 write(1,&ch,1);
		}

		// Print spaces on top
		for (i =0; i < line_length; i++) {
		  ch = ' ';
	 	 write(1,&ch,1);
		}

		// Print backspaces
		for (i =0; i < line_length; i++) {
	 	 ch = 8;
	  	write(1,&ch,1);
		}	

		// Copy line from history
		strcpy(line_buffer, history[history_index - 1]);
		line_length = strlen(line_buffer);
		history_index --;

		cursor_pos = line_length;
		// echo line
		write(1, line_buffer, line_length);
	}
      }

      /*down arrow*/
      if (ch1==91 && ch2==66) {

      if (history_index != history_length) {
      
      	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

	// Copy line from history
	strcpy(line_buffer, history[history_index + 1]);
	line_length = strlen(line_buffer);
	history_index ++;
	cursor_pos = line_length;

	// echo line
	write(1, line_buffer, line_length);

      	}
      }

      /*right arrow*/
      if (ch1==91 && ch2==67) {

      	if (cursor_pos < line_length) {
      	
	 ch = line_buffer[cursor_pos];
	 write(1, &ch, 1);
	 cursor_pos ++;
	}
      }
      /*left arrow*/
      if (ch1==91 && ch2==68) {
      	
	  if (cursor_pos >= 1) {
		ch = 8;
	 	write(1,&ch,1);
		cursor_pos --;
	}
      }
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  FILE * his = fopen("history.txt", "a");
  fwrite(line_buffer, sizeof(char), line_length, his);
  fclose(his);

  history_index = 0;
  history_length = 0;
  
  tcsetattr(0, TCSANOW, &orig_attr);
  return line_buffer;
}

