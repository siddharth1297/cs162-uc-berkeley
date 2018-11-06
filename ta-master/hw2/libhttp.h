/*
 * A simple HTTP library.
 *
 * Usage example:
 *
 *     // Returns NULL if an error was encountered.
 *     struct http_request *request = http_request_parse(fd);
 *
 *     ...
 *
 *     http_start_response(fd, 200);
 *     http_send_header(fd, "Content-type", http_get_mime_type("index.html"));
 *     http_send_header(fd, "Server", "httpserver/1.0");
 *     http_end_headers(fd);
 *     http_send_string(fd, "<html><body><a href='/'>Home</a></body></html>");
 *
 *     close(fd);
 */

#ifndef LIBHTTP_H
#define LIBHTTP_H

/*
 * Functions for parsing an HTTP request.
 */
struct http_request {
  char *method;
  char *path;
};

char* full_file_name;
struct http_request *http_request_parse(int fd);

/*
 * Functions for sending an HTTP response.
 */
void http_start_response(int fd, int status_code);
void http_send_header(int fd, char *key, char *value);
void http_end_headers(int fd);
void http_send_string(int fd, char *data);
void http_send_data(int fd, char *data, size_t size);

/*
 * Helper function: gets the Content-Type based on a file name.
 */
char *http_get_mime_type(char *file_name);

/* Check directory or not */
int check_directory(char *dir_name);

/* Check file or not */
int check_file(char *name);

/* Check whether contains index.html */
int contain_file(char *file_path);

/* Read files/directories from directory */
char *read_directory(char *dir_name);

/* Read whole path starting from directory */
char *read_path(char*, char*);

/* content length of file */
size_t get_content_length(char *file_name);

/* Content of a file */
char* get_content_string(char *file_name);

#endif
