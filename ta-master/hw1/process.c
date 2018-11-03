#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "tokenizer.h"

/* Check for background process and set group id accordingly */
int put_background(struct tokens* tokens) {

	if(strcmp(tokens_get_token(tokens, tokens_get_length(tokens)-1), "&") == 0) {
		if (setpgid(0, 0) == -1) {
      		fprintf(stderr, "%s\n", strerror(errno));
		  }
		return 1;	
	}
	return 0;
}