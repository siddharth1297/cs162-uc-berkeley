#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "libhttp.h"

#define LIBHTTP_REQUEST_MAX_SIZE 8192


void http_fatal_error(char *message) {
  fprintf(stderr, "%s\n", message);
  exit(ENOBUFS);
}

struct http_request *http_request_parse(int fd) {
  struct http_request *request = malloc(sizeof(struct http_request));
  if (!request) http_fatal_error("Malloc failed");

  char *read_buffer = malloc(LIBHTTP_REQUEST_MAX_SIZE + 1);
  if (!read_buffer) http_fatal_error("Malloc failed");

  int bytes_read = read(fd, read_buffer, LIBHTTP_REQUEST_MAX_SIZE);
  read_buffer[bytes_read] = '\0'; /* Always null-terminate. */

  char *read_start, *read_end;
  size_t read_size;

  do {
    /* Read in the HTTP method: "[A-Z]*" */
    read_start = read_end = read_buffer;
    while (*read_end >= 'A' && *read_end <= 'Z') read_end++;
    read_size = read_end - read_start;
    if (read_size == 0) break;
    request->method = malloc(read_size + 1);
    memcpy(request->method, read_start, read_size);
    request->method[read_size] = '\0';

    /* Read in a space character. */
    read_start = read_end;
    if (*read_end != ' ') break;
    read_end++;

    /* Read in the path: "[^ \n]*" */
    read_start = read_end;
    while (*read_end != '\0' && *read_end != ' ' && *read_end != '\n') read_end++;
    read_size = read_end - read_start;
    if (read_size == 0) break;
    request->path = malloc(read_size + 1);
    memcpy(request->path, read_start, read_size);
    request->path[read_size] = '\0';

    /* Read in HTTP version and rest of request line: ".*" */
    read_start = read_end;
    while (*read_end != '\0' && *read_end != '\n') read_end++;
    if (*read_end != '\n') break;
    read_end++;

    free(read_buffer);
    return request;
  } while (0);

  /* An error occurred. */
  free(request);
  free(read_buffer);
  return NULL;

}

char* http_get_response_message(int status_code) {
  switch (status_code) {
    case 100:
      return "Continue";
    case 200:
      return "OK";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 304:
      return "Not Modified";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    default:
      return "Internal Server Error";
  }
}

void http_start_response(int fd, int status_code) {
  dprintf(fd, "HTTP/1.0 %d %s\r\n", status_code,
      http_get_response_message(status_code));
}

void http_send_header(int fd, char *key, char *value) {
  dprintf(fd, "%s: %s\r\n", key, value);
}

void http_end_headers(int fd) {
  dprintf(fd, "\r\n");
}

void http_send_string(int fd, char *data) {
  http_send_data(fd, data, strlen(data));
}

void http_send_data(int fd, char *data, size_t size) {
  ssize_t bytes_sent;
  while (size > 0) {
    bytes_sent = write(fd, data, size);
    if (bytes_sent < 0)
      return;
    size -= bytes_sent;
    data += bytes_sent;
  }
}

char *http_get_mime_type(char *file_name) {
  char *file_extension = strrchr(file_name, '.');
  if (file_extension == NULL) {
    return "text/plain";
  }

  if (strcmp(file_extension, ".html") == 0 || strcmp(file_extension, ".htm") == 0) {
    return "text/html";
  } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
    return "image/jpeg";
  } else if (strcmp(file_extension, ".png") == 0) {
    return "image/png";
  } else if (strcmp(file_extension, ".css") == 0) {
    return "text/css";
  } else if (strcmp(file_extension, ".js") == 0) {
    return "application/javascript";
  } else if (strcmp(file_extension, ".pdf") == 0) {
    return "application/pdf";
  } else {
    return "text/plain";
  }
}

/* Check directory or not */
int check_directory(char *dir_name) {
    struct stat dir_stat;
    if(stat(dir_name, &dir_stat) == -1) {
      fprintf(stderr, "Check directory %s : %s\n", dir_name, strerror(errno));
      return 0;
    }
    return S_ISDIR(dir_stat.st_mode);
}

/* Check file or not */
int check_file(char *file_name) {
  struct stat f_info;
  if(stat(file_name, &f_info) == -1) {
    fprintf(stderr, "Check file %s : %s\n", file_name, strerror(errno));
    return 0;
  }
  return S_ISREG(f_info.st_mode);
}

/* Check whether contains index.html */
int contain_file(char *file_path) {
  char *f_n;
  if((f_n=(char*)malloc(sizeof(char*)*(strlen(file_path) + strlen("index.html")+2))) == NULL) {
    fprintf(stderr, "%s\n", strerror((errno)));
    return 0;
  }

  strcpy(f_n, file_path);
  strcat(f_n, "index.html");
        
  if(check_file(f_n)) {
    full_file_name = (char*) malloc(sizeof(char*)*(strlen(f_n)+1));
    full_file_name = strdup(f_n);
    free(f_n);
    return 1;
  }
  return 0;
}

/* Read whole path starting from directory */
char *read_path(char *directory, char* path) {
  char *full_path;
  if((full_path = (char *)malloc(sizeof(char)*(strlen(path)+strlen(directory)+2))) == NULL) {
    fprintf(stderr, "%s\n", strerror(errno));
    return NULL;
  }

  strcpy(full_path, directory);
  strcat(full_path, path);

  char last_char = full_path[strlen(full_path)-1];

  if(last_char == '/') {
    /* Directory */
    if(check_directory(full_path)) {
      /* Check for file */
      if(contain_file(full_path)) {
        /* Return file content */
        free(full_path);
        return get_content_string(full_file_name);
      } else {
        /* Directory exist, but file not found */
        return read_directory(full_path);
      }
    } else {
      /* Directory not exist */
      return NULL;
    }
  }

  if(check_file(full_path)) {
    /* File found */
    if((full_file_name = (char*)malloc(sizeof(char)*(strlen(full_path)+1))) == NULL) {
      fprintf(stderr, "%s\n", strerror(errno));
      return NULL;
    }

    strcpy(full_file_name, full_path);
    return get_content_string(full_file_name);
  } else {
    /* File not found */
    return NULL;
  }

  return NULL;
}

/* Read files/directories from directory */
char *read_directory(char *dir_name) {
  DIR *dir;

  if((dir = opendir(dir_name)) == NULL) {
    fprintf(stderr, "Error in openning directory : %s\n", strerror(errno));
    return NULL;
  }
  
  char *str = (char*)malloc(1024);
  if(str == NULL) {
    fprintf(stderr, "%s\n", strerror(errno));
    return NULL;
  }
  
  struct dirent *d;
  
  char *s = (char *)malloc(1024);
  sprintf(s, " Index of %s <br> ", dir_name);
  strcpy(str, s);
  free(s);
  while((d = readdir(dir)) != 0) {
    s = (char *)malloc(1024);
    if(strcmp(d->d_name, "..") == 0)
      sprintf(s, " <a href=\"%s\"> %s </a> <br> ", d->d_name ,"Parent Directory");
    else
      sprintf(s, " <a href=\"%s\"> %s </a> <br> ", d->d_name ,d->d_name);
    strcat(str, s);
    free(s);
  }

  closedir(dir);
    
  return str;
}

/* Content length of file */
size_t get_content_length(char *file_name) {
   struct stat f_info;
   if(stat(file_name, &f_info) == -1) {
    fprintf(stderr, "Get content length %s : %s\n", file_name, strerror(errno));
    return 0;
  }
  return f_info.st_size;
}

/* Content of a file */
char* get_content_string(char *file_name) {
  size_t length = get_content_length(file_name);
  FILE *fp = fopen(file_name, "r");
  if(fp == NULL) {
    fprintf(stderr, "Error in opening file : %s\n", strerror(errno));
    return NULL;
  }
  char *str = malloc(length);
  if(fread(str, 1, length, fp) < length) {
      fprintf(stderr, "ERROR: failed to read file\n");
      return NULL;
   }
   return str;
}