#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

// Declare data type for counts
#define count_t unsigned long long int 

//total count for multiple files
static count_t	tot_line_cnt, tot_word_cnt, tot_char_cnt;

//count line, word, char
void count(char*);

//print line, word, char
void print();

int main(int argc, char *argv[]) {
    	
	if(argc == 1) {
		//no argument is passed- read from stdin
		count(NULL);
	} else {
		//count for each file
		int i=1;
		while(i<argc) {
			//count 
			count(argv[i]);
			i++;
		}
		//print total, if more than one file
		if(argc > 2)
			print("total", tot_line_cnt, tot_word_cnt, tot_char_cnt);
	}

	return 0;
}

void count(char* file) {

	//counts for file
	count_t line_cnt = 0, word_cnt = 0, char_cnt = 0;

	int fd;
	
	if(file == NULL) {
		//read from stdin
		fd = STDIN_FILENO;
	} else {
		//open file
		if((fd = open(file, O_RDONLY)) < 0 ) {
			//print error message
			fprintf(stderr, "wc: %s: No such file or directory\n", file);
			return ;
		}

	}
	
	//read from the file
	char c;
	int has_space = 1;
	while(read(fd, &c, 1) == 1) {
		//check if space
		if(isspace(c)) { //if space
			has_space = 1; //enable space
			if(c == '\n') //if end of line, increment line count
				line_cnt++;
		} else { //if not space 
			if(has_space) { //if space is enabled
				has_space = 0; //disable space
				word_cnt++; //increment word count
			}
		}
		//increment char count
		char_cnt++;
	}

	//print
	print(file, line_cnt, word_cnt, char_cnt);

	//update tottal counts
	tot_line_cnt += line_cnt;
	tot_word_cnt += word_cnt;
	tot_char_cnt += char_cnt;
}

void print(char *file, count_t line_cnt, count_t word_cnt, count_t char_cnt) {
	
	printf(" %llu\t%llu\t%llu\t", line_cnt, word_cnt, char_cnt);
	//if read from stdin, nothing to be printed
	if(file == NULL)
		printf("\n");
	else //read from files, print file name or total(for multiple files)
		printf("%s\n", file);
}
