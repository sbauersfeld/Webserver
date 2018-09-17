#include <stdio.h>
#include <sys/types.h>   
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>

/** Returns the true case-sensitive filename corresponding to the requested
filename, or null if the file is not found. Example: client requests file.txt,
and this function returns FiLe.txt if that is the real name of the file in the
directory. */
char* get_true_filename(char* requestedFile) {
  /* Open the current directory. */
  DIR *dir_ptr;
  struct dirent *dirent_ptr;
  if ((dir_ptr = opendir(".")) == NULL) {
    fprintf(stderr, "System call opendir failed. Error value is %s\n", strerror(errno));
    exit(1);
  }

  /* Iterate through each directory entry in the current directory. */
  while ((dirent_ptr = readdir(dir_ptr)) != NULL) {
    /* Convert directory entry to lower case. The requested filename is
    already in lower case after parse_request() is called, so it does not
    require conversion. */
    char dirfile[strlen(dirent_ptr->d_name + 1)];
    int i;
    for (i = 0; dirent_ptr->d_name[i] != '\0'; i++) {
      dirfile[i] = tolower(dirent_ptr->d_name[i]);
    }
    dirfile[i] = '\0';

    /* Compare directory entry to requested filename */
    if (strcmp(dirfile, requestedFile) == 0) {
      /* If file is found, return its true name (case-sensitive) */
      char *trueName = malloc(strlen(dirent_ptr->d_name + 1));
      strcpy(trueName, dirent_ptr->d_name);
      closedir(dir_ptr);
      return trueName;
    }
  }
  /* Close the directory. */
  closedir(dir_ptr);
  return NULL;
}

/** Sends the HTTP response message header to the client. */
void send_header(int size, char* file, int client_sock_num){
  /* Initialize header fields. */
  char header[400] = "HTTP/1.1 200 OK\r\n"; //status codes
  char file_size[100] = "Content-Length: "; //file size 
  char len_buffer[50];
  sprintf(len_buffer, "%d", size);
  strcat(file_size, len_buffer);
  strcat(file_size, "\r\n");
  
  /* Initialize data field in header */
  char date[100] = "Date: ";
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  char curtime[50];
  strftime(curtime, 32, "%a, %d %b %Y %T %Z", timeinfo);
  strcat(date, curtime); 
  strcat(date, "\r\n");

  char* holder; 
  char file_type[50] = "Content-Type: "; //content type determined by extension in file name
  
  /* Determine the correct file extension. Use binary file type if unsupported
  extension or no extension provided. */
  if ((holder = strchr(file, '.')) != NULL){
    if (strcasecmp(holder, ".jpg") == 0 || strcasecmp(holder, ".jpeg") == 0)
      strcat(file_type, "image/jpeg\r\n");
    else if (strcasecmp(holder, ".gif") == 0)
      strcat(file_type, "image/gif\r\n");
    else if (strcasecmp(holder, ".html") == 0 || strcasecmp(holder, ".htm") == 0)
      strcat(file_type, "text/html\r\n");
    else if (strcasecmp(holder, ".txt") == 0)
      strcat(file_type, "text/plain\r\n");
    else strcat(file_type, "application/octet-stream\r\n");
  }
  else strcat(file_type, "application/octet-stream\r\n");

  char* close = "Connection: close\r\n\r\n";

  /* Combine all header fields into the header. */
  strcat(header, file_size);
  strcat(header, date);
  strcat(header, file_type);
  strcat(header, close);

  /* Send the HTTP response header to the client. */
  if (write(client_sock_num, header, strlen(header)) == -1){
        fprintf(stderr, "System call write failed. Error value is %s\n", strerror(errno));
        exit(1);
   }
}

/** Parses HTTP request message to extract and return the requested filename in
all lower-case characters. */
char* parse_request(char* buffer){
  char* file;
  int size = 1024;
  /* Assert that the first line in HTTP request contains "GET /" */
  if (buffer[0] != 'G' || buffer[1] != 'E' || buffer[2] != 'T' || buffer[3] != ' ' || buffer[4] != '/') {
    exit(0);
  }

  /* Extract the filename */
  file = (char*) malloc(sizeof(char)*size);
  int index = 5, pos = 0;
  int cap = size;
  while (buffer[index] != ' ') {
    if (pos == cap){
      cap*=2;
      if ( (file = (char*) realloc(file, sizeof(char)*cap)) == NULL) {
          fprintf(stderr, "Realloc failed to allocate memory\n.");
          exit(2);
       }
    }

    /* Convert '%20' to space character ' ' */
    if (buffer[index]=='%' && buffer[index+1]=='2' && buffer[index+2]=='0') {
      file[pos]=' ';
      index+=3;
      pos+=1;
    } else {
      /* Add non-space characters to filename string. */
      file[pos]=tolower(buffer[index]);
      index+=1;
      pos+=1;
    }
  }
  file[pos] = '\0';
  return file;
}

/** Sends a 404 file-not-found error message to the client. */
void send_404(int client_sock_num){
  /* Initialize the HTTP response message header and error message. */
  char header[400] = "HTTP/1.1 404 Not Found\r\n";
  char file_size[50] = "Content-Length: "; //file size
  char len_buf[20];
  char* msg = "Error 404: The requested file could not be found on this server.\n";
  int len = strlen(msg);

  sprintf(len_buf, "%d", len);
  strcat(file_size, len_buf);
  strcat(file_size, "\r\n");

  /* Initialize the date field for HTTP header. */
  char date[100] = "Date: ";
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  char curtime[50];
  strftime(curtime, 32, "%a, %d %b %Y %T %Z", timeinfo);
  strcat(date, curtime);
  strcat(date, "\r\n");
  
  char* type = "Content-Type: text/plain\r\n";
  char* close = "Connection: close\r\n\r\n";

  /* Combine header fields into response header. */
  strcat(header, type);
  strcat(header, file_size);
  strcat(header, date);
  strcat(header, close);
  strcat(header, msg);

  /* Send response message to client. */
  if (write(client_sock_num, header, strlen(header)) == -1){
        fprintf(stderr, "System call write failed. Error value is %s\n", strerror(errno));
        exit(1);
   }
}

/** Handles an HTTP request: parse request messages and send HTTP response. */
void serve_request(int client_sock_num){
  /* Store HTTP request message in a buffer to be written out later. */
  int size = 4096;
  char buffer[size];
  int r = 0;
  r = read(client_sock_num, buffer, size);
  if (r < 0){
    fprintf(stderr, "System call read failed. Error value is %s\n", strerror(errno));
    exit(1);
  }

  /* Write the HTTP request message to the Terminal. */
  if (write(1, buffer, r) == -1){
      fprintf(stderr, "System call write failed. Error value is %s\n", strerror(errno));
      exit(1);
  }

  /* Extract lower-case filename from HTTP request. */
  int fd = 0;
  char* file = parse_request(buffer);
  char* trueFile;

  /* Get the real case-sensitive filename matching the directory entry. */
  trueFile = get_true_filename(file);
  if (trueFile == NULL){
    send_404(client_sock_num);
    if (file) free(file);
    return;
  }

  /* Open the requested file in the directory. */
  if ((fd = open(trueFile, O_RDONLY)) == -1){
    fprintf(stderr, "System call open failed. Error value is %s\n", strerror(errno));
    exit(1);
  }

  /* Find the size of the requested file. */
  struct stat file_stats;
  fstat(fd, &file_stats);
  int size2 = file_stats.st_size;

  /* Send the header for the HTTP response message to the client. */
  send_header(size2, trueFile, client_sock_num);

  /* Read the file data and send to client in chunks as HTTP response data. */
  char buffer2[size];
  while ( (r = read(fd, buffer2, size)) > 0){
    if (write(client_sock_num, buffer2, r) == -1){
      fprintf(stderr, "System write open failed. Error value is %s\n", strerror(errno));
      exit(1);
    }
  }
  if (r < 0) {
    fprintf(stderr, "System call read failed. Error value is %s\n", strerror(errno));
    exit(1);
  }

  /* Free dynamically allocated memory. */
  if (trueFile) free(trueFile);
  if (file) free(file);
}

int main(void){
  /* Initialize the socket used to communicate with client. */
  int sock_num;
  sock_num = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_num == -1){
    fprintf(stderr, "System call socket failed. Error value is: %s\n", strerror(errno));
    exit(1);
  }

  /* Bind to socket using port 2222. */
  struct sockaddr_in server, client;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  server.sin_port = htons(2222);
  if (bind(sock_num,(struct sockaddr *)&server, sizeof(server)) == -1){
    fprintf(stderr, "System call bind failed. Error value is: %s\n", strerror(errno));
    exit(1);
  }

  /* Listen for incoming requests. */
  if (listen(sock_num, 5) == -1){
    fprintf(stderr, "System call listen failed. Error value is: %s\n", strerror(errno));
    exit(1);
  }

  int client_sock_num;
  int addr_size = sizeof(struct sockaddr_in);

  while(1){
    /* Accept incoming requests. */
    client_sock_num = accept(sock_num, (struct sockaddr*) &client, (socklen_t*) &addr_size);
    if (client_sock_num == -1){
      fprintf(stderr, "System call accept failed. Error value is: %s\n", strerror(errno));
      exit(1);
    }
    /* Create child process to serve each request. */
    if (fork() == 0){
      close(sock_num);
      serve_request(client_sock_num);
      close(client_sock_num);
      exit(0);
    }
    else {
      close(client_sock_num);
    }
  }
}
