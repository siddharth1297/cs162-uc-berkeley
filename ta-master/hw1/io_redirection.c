#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "tokenizer.h"
#include "io_redirection.h"

/* Redirect */
int redirect(struct tokens *tokens, size_t tok_len) {
	size_t i;
	int fd, newfd;

	for(i=0; i< tok_len; i++) {
		/* Check for op redirection */
		if(strcmp(tokens_get_token(tokens, i), ">" ) == 0) {
			if(i+1 == tok_len-1) { 
				/* Redirect */
				fd = open(tokens_get_token(tokens, i+1), O_CREAT|O_WRONLY|O_TRUNC, 0644);

				if(fd == -1) {
					fprintf(stderr, "%s\n", strerror(errno));
					return -1;
				}

				/* Duplicate */
				newfd = dup2(fd, STDOUT_FILENO);
		
				if(newfd == -1) {
					fprintf(stderr, "%s\n", strerror(errno));
					return -1;
				}
			} else {
				/* Syntax Error */
				fprintf(stderr, "%s\n", "Syntax Error");
				break;
			}
			redirect_opt = 1;
			return i;
		}

		/* Check for ip redirection */
		if(strcmp(tokens_get_token(tokens, i), "<" ) == 0) {
			if(i+1 == tok_len-1) {
				/* Redirect */
				fd = open(tokens_get_token(tokens, i+1), O_RDONLY, 0644);
				if(fd == -1) {
					fprintf(stderr, "%s\n", strerror(errno));
					return -1;
				}
				
				/* Duplicate */
				newfd = dup2(fd, STDIN_FILENO);
				if(newfd == -1) {
					fprintf(stderr, "%s\n", strerror(errno));
					return -1;
				}
			} else {
				/* Syntax Error */
				fprintf(stderr, "%s\n", "Syntax Error");
				break;
			}
			redirect_opt = -1;
			return i;
		}
	}
	redirect_opt = 0;
	return -1;
}