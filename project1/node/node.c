#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#define BUFFER_SIZE 8192
#define MAX_FILES 1000
#define MAX_PATH_LENGTH 1024
#define TIMEOUT_SECONDS 60 // 1 min timeout

typedef struct {
    char path[MAX_PATH_LENGTH];
    size_t size;
    time_t modified_time;
} FileInfo; // infof or all files

FileInfo indexed_files[MAX_FILES];
int file_count = 0;
char index_directory[MAX_PATH_LENGTH];

// b64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *encode_base64(const unsigned char *data, size_t input_len, size_t *output_len) { // basically gotten from here for reference: https://github.com/jwerle/b64.c
    *output_len = 4 * ((input_len + 2) / 3);
    char *encoded_data = (char *)malloc(*output_len + 1);
    if (!encoded_data) {
        return NULL;
    }
    size_t i, j;
    for (i = 0, j = 0; i < input_len;) {
        uint32_t octet_a = i < input_len ? data[i++] : 0;
        uint32_t octet_b = i < input_len ? data[i++] : 0;
        uint32_t octet_c = i < input_len ? data[i++] : 0;
        
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        encoded_data[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded_data[j++] = base64_table[triple & 0x3F];
    }
    for (i = 0; i < (3 - input_len % 3) % 3; i++) {
        encoded_data[*output_len - 1 - i] = '=';
    }
    encoded_data[*output_len] = '\0';
    return encoded_data;
}

void index_files(const char *dir_path, const char *relative_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }
    
    struct dirent *entry;
    char full_path[MAX_PATH_LENGTH];
    char file_path[MAX_PATH_LENGTH];
    
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (strlen(relative_path) == 0) {
            snprintf(file_path, sizeof(file_path), "%s", entry->d_name);
        } else {
            snprintf(file_path, sizeof(file_path), "%s/%s", relative_path, entry->d_name);
        }
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                index_files(full_path, file_path);
            } else if (S_ISREG(st.st_mode)) {
                strncpy(indexed_files[file_count].path, file_path, MAX_PATH_LENGTH - 1);
                indexed_files[file_count].size = st.st_size;
                indexed_files[file_count].modified_time = st.st_mtime;
                file_count++;
                
                printf("Indexed: %s (%zu bytes)\n", file_path, (size_t)st.st_size);
            }
        }
    }
    closedir(dir);
}
int match_pattern(const char *pattern, const char *filename) {
    const char *wildcard = strchr(pattern, '*');
    if (!wildcard) {
        return strcmp(pattern, filename) == 0;
    }
    size_t prefix_len = wildcard - pattern;
    const char *suffix = wildcard + 1;
    size_t suffix_len = strlen(suffix);
    if (strncmp(pattern, filename, prefix_len) != 0) {
        return 0;
    }
    size_t filename_len = strlen(filename);
    if (suffix_len > filename_len - prefix_len) {
        return 0;
    }
    return strcmp(suffix, filename + filename_len - suffix_len) == 0;
}

void handle_helo(int client_socket, int server_port) {
    char response[BUFFER_SIZE];
    
    snprintf(response, sizeof(response),
             "200 OK\r\n"
             "Port: %d\r\n"
             "Content-Directory: %s\r\n"
             "Indexed-Files: %d\r\n"
             "\r\n",
             server_port, index_directory, file_count);
    
    send(client_socket, response, strlen(response), 0);
}

void handle_find(int client_socket, const char *pattern) {
    char response[BUFFER_SIZE];
    char matches[BUFFER_SIZE];
    int match_count = 0;
    
    memset(matches, 0, sizeof(matches));
    
    for (int i = 0; i < file_count; i++) {
        if (match_pattern(pattern, indexed_files[i].path)) {
            struct tm *timeinfo = localtime(&indexed_files[i].modified_time);
            char date_str[20];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);
            char match_info[MAX_PATH_LENGTH + 50];
            snprintf(match_info, sizeof(match_info), "%d;%s;%zu;%s\r\n",
                     i + 1, indexed_files[i].path, indexed_files[i].size, date_str);
            if (strlen(matches) + strlen(match_info) < sizeof(matches) - 1) {
                strcat(matches, match_info);
                match_count++;
            }
        }
    }
    snprintf(response, sizeof(response),
             "200 OK\r\n"
             "Search-Pattern: %s\r\n"
             "Matched-Files: %d\r\n"
             "\r\n"
             "%s"
             "\r\n",
             pattern, match_count, matches);
    
    send(client_socket, response, strlen(response), 0);
}
void handle_get(int client_socket, int file_number) {
    char header[BUFFER_SIZE];
    if (file_number <= 0 || file_number > file_count) {
        snprintf(header, sizeof(header),
                 "400 Bad Request\r\n"
                 "Error: Invalid file number\r\n"
                 "\r\n");
        send(client_socket, header, strlen(header), 0);
        return;
    }
    FileInfo *file_info = &indexed_files[file_number - 1];
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", index_directory, file_info->path);
    FILE *file = fopen(full_path, "rb");
    if (!file) {
        snprintf(header, sizeof(header),
                 "400 Bad Request\r\n"
                 "Error: Cannot open file\r\n"
                 "\r\n");
        send(client_socket, header, strlen(header), 0);
        return;
    }
    unsigned char *file_content = (unsigned char *)malloc(file_info->size);
    if (!file_content) {
        fclose(file);
        snprintf(header, sizeof(header),
                 "400 Bad Request\r\n"
                 "Error: Memory allocation failed\r\n"
                 "\r\n");
        send(client_socket, header, strlen(header), 0);
        return;
    }
    size_t bytes_read = fread(file_content, 1, file_info->size, file);
    fclose(file);
    if (bytes_read != file_info->size) {
        free(file_content);
        snprintf(header, sizeof(header),
                 "400 Bad Request\r\n"
                 "Error: Failed to read file\r\n"
                 "\r\n");
        send(client_socket, header, strlen(header), 0);
        return;
    }
    size_t encoded_size;
    char *encoded_content = encode_base64(file_content, file_info->size, &encoded_size);
    free(file_content);
    if (!encoded_content) {
        snprintf(header, sizeof(header),
                 "400 Bad Request\r\n"
                 "Error: Failed to encode file\r\n"
                 "\r\n");
        send(client_socket, header, strlen(header), 0);
        return;
    }
    struct tm *timeinfo = localtime(&file_info->modified_time);
    char date_str[20];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);
    snprintf(header, sizeof(header),
             "200 OK\r\n"
             "Request-File: %d\r\n"
             "File-Name: %s\r\n"
             "File-Size: %zu\r\n"
             "File-Date: %s\r\n"
             "Encoded-Size: %zu\r\n"
             "\r\n",
             file_number, file_info->path, file_info->size, date_str, encoded_size);
    size_t total_size = strlen(header) + encoded_size + 2; // header + content + \r\n
    char *complete_response = malloc(total_size + 1);
    if (!complete_response) {
        free(encoded_content);
        snprintf(header, sizeof(header),
                 "400 Bad Request\r\n"
                 "Error: Memory allocation failed\r\n"
                 "\r\n");
        send(client_socket, header, strlen(header), 0);
        return;
    }
    strcpy(complete_response, header);
    memcpy(complete_response + strlen(header), encoded_content, encoded_size);
    strcpy(complete_response + strlen(header) + encoded_size, "\r\n");
    send(client_socket, complete_response, total_size, 0);
    
    free(complete_response);
    free(encoded_content);
}

void handle_end(int client_socket) {
    char response[] = "200 OK\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
}

void handle_client(int client_socket, int server_port) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int timeout_counter = 0;
    
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SECONDS;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("Client disconnected\n");
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Client timed out\n");
            } else {
                perror("recv failed");
            }
            break;
        }
        
        buffer[bytes_read] = '\0';
        printf("Received: %s", buffer);
        char *command_end = strstr(buffer, "\r\n");
        if (!command_end) {
            char response[] = "400 Bad Request\r\nError: Malformed command\r\n\r\n";
            send(client_socket, response, strlen(response), 0);
            continue;
        }
        *command_end = '\0';
        if (strncmp(buffer, "HELO", 4) == 0) {
            handle_helo(client_socket, server_port);
        } else if (strncmp(buffer, "FIND", 4) == 0) {
            char *pattern = buffer + 4;
            while (*pattern && isspace(*pattern)) pattern++;
            
            if (*pattern) {
                handle_find(client_socket, pattern);
            } else {
                char response[] = "400 Bad Request\r\nError: Missing search pattern\r\n\r\n";
                send(client_socket, response, strlen(response), 0);
            }
        } else if (strncmp(buffer, "GET", 3) == 0) {
            char *num_str = buffer + 3;
            while (*num_str && isspace(*num_str)) num_str++;
            
            if (*num_str) {
                int file_number = atoi(num_str);
                handle_get(client_socket, file_number);
            } else {
                char response[] = "400 Bad Request\r\nError: Missing file number\r\n\r\n";
                send(client_socket, response, strlen(response), 0);
            }
        } else if (strncmp(buffer, "END", 3) == 0) {
            handle_end(client_socket);
            break;
        } else {
            char response[] = "400 Bad Request\r\nError: Unknown command\r\n\r\n";
            send(client_socket, response, strlen(response), 0);
        }
    }
    
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <index directory>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    strncpy(index_directory, argv[2], sizeof(index_directory) - 1);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        return 1;
    }
    struct stat st;
    if (stat(index_directory, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Index directory does not exist: %s\n", index_directory);
        return 1;
    }
    printf("Indexing files in %s...\n", index_directory);
    index_files(index_directory, "");
    printf("Indexed %d files\n", file_count);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("Server listening on port %d\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        handle_client(client_socket, port);
        
        printf("Client disconnected\n");
    }
    
    close(server_fd);
    return 0;
}