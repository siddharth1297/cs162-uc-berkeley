#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "path_resolution.h"

/* Returns full path */
char *file_path_exist(char *path, char *prog) {

	char *full_path = (char *) malloc((strlen(path) + strlen(prog) + 2) * sizeof(char));

	strcat(full_path, path);
	strcat(full_path, "/");
	strcat(full_path, prog);

	if(access(full_path, X_OK) == 0)
		return full_path;

	return NULL;
}

/* Creates all possible paths */
char *find_full_path(char *prog) {
	char *PATH = (char *) malloc(1024*sizeof(char));
	strcpy(PATH, getenv("PATH"));

	char *path_dup = (char *) malloc(1024 * sizeof(char)), *p_d = (char *) malloc(1024 * sizeof(char));
	
	path_dup = strdup(PATH);
	p_d = strdup(PATH);
	
	while(path_dup != NULL) {
		
		/* break */
		strtok((path_dup), ":");

		/* Check */
		char *f_path = (char *) malloc(1024 *sizeof(char));
		f_path = file_path_exist(path_dup, prog);
		if(f_path != NULL) {
			/* retrun full path */
			return f_path;
		} else {
			free(f_path);
		}

		p_d = strchr(p_d, ':');
		if(p_d == NULL)
			break;
		
		p_d = strdup(p_d+1);
		path_dup = strdup(p_d);
	}

	return prog;
}