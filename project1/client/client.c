#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <ctype.h>

#define BUFFER_SIZE 8192
#define MAX_COMMAND_SIZE 1024
#define CONNECTION_TIMEOUT 10 // 10 sec timeout

// b64 decoding table
static const unsigned char base64_decode_table[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};
int directory_exists(const char *path) { // check if directory exists
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int create_directory_path(const char *path) { // create all the diretories if needed in path
    char *path_copy = strdup(path);
    char *p = path_copy;
    int status = 0;

    if (*p == '/') {
        p++;
    }

    while (*p != '\0') {
        if (*p == '/') {
            *p = '\0'; 
            
            if (!directory_exists(path_copy) && mkdir(path_copy, 0755) != 0) {
                status = -1;
                break;
            }
            
            *p = '/';
        }
        p++;
    }

    if (status == 0 && !directory_exists(path_copy) && mkdir(path_copy, 0755) != 0) {
        status = -1;
    }

    free(path_copy);
    return status;
}

int create_parent_directories(const char *file_path) {
    char *path_copy = strdup(file_path);
    char *dir = dirname(path_copy);
    int status = create_directory_path(dir);
    free(path_copy);
    return status;
}

unsigned char *decode_base64(const char *input, size_t *output_len) { // basically copied from here for reference: https://github.com/jwerle/b64.c
    size_t input_len = strlen(input);
    char *cleaned_input = (char *)malloc(input_len + 1);
    if (!cleaned_input) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    size_t cleaned_len = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (!isspace((unsigned char)input[i])) {
            cleaned_input[cleaned_len++] = input[i];
        }
    }
    cleaned_input[cleaned_len] = '\0';
    size_t pad_count = 0;
    if (cleaned_len > 0 && cleaned_input[cleaned_len - 1] == '=') pad_count++;
    if (cleaned_len > 1 && cleaned_input[cleaned_len - 2] == '=') pad_count++; // count num of =
    *output_len = (cleaned_len * 3) / 4 - pad_count;
    unsigned char *output = (unsigned char *)malloc(*output_len);
    if (!output) {
        free(cleaned_input);
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    size_t i, j;
    unsigned int sextet[4];
    for (i = 0, j = 0; i < cleaned_len; i += 4) {
        for (int k = 0; k < 4; k++) {
            if (i + k < cleaned_len && cleaned_input[i + k] != '=') {
                sextet[k] = base64_decode_table[(unsigned char)cleaned_input[i + k]];
                if (sextet[k] == 64) {  
                    fprintf(stderr, "Invalid base64 character: %c\n", cleaned_input[i + k]);
                    free(cleaned_input);
                    free(output);
                    *output_len = 0;
                    return NULL;
                }
            } else {
                sextet[k] = 0;
            }
        }
        if (j < *output_len) {
            output[j++] = (sextet[0] << 2) | (sextet[1] >> 4);
        }
        if (j < *output_len) {
            output[j++] = (sextet[1] << 4) | (sextet[2] >> 2);
        }
        if (j < *output_len) {
            output[j++] = (sextet[2] << 6) | sextet[3];
        }
    }
    
    free(cleaned_input);
    return output;
}

char *receive_complete_response(int sockfd, int *status_code) {
    char buffer[BUFFER_SIZE];
    char *response = NULL;
    size_t total_size = 0;
    size_t response_size = 0;
    int bytes_received;
    int found_end = 0;
    
    *status_code = 0;
    
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        response = realloc(response, total_size + bytes_received + 1);
        if (!response) {
            fprintf(stderr, "Memory allocation failed\n");
            return NULL;
        }
        memcpy(response + total_size, buffer, bytes_received + 1);
        total_size += bytes_received;
        
        if (*status_code == 0 && sscanf(response, "%d", status_code) != 1) {
            *status_code = 0; 
        }
        if (strstr(response, "\r\n\r\n")) {
            if (strstr(response, "GET") == NULL) {
                found_end = 1;
                break;
            } else {
                char *headers_end = strstr(response, "\r\n\r\n");
                if (headers_end) {
                    headers_end += 4; 
                    response_size = total_size - (headers_end - response);
                    char *encoded_size_header = strstr(response, "Encoded-Size:");
                    if (encoded_size_header) {
                        size_t encoded_size = 0;
                        sscanf(encoded_size_header, "Encoded-Size: %zu", &encoded_size);
                        if (response_size >= encoded_size) {
                            found_end = 1;
                            break;
                        }
                    } else {
                        found_end = 1;
                        break;
                    }
                }
            }
        }
    }
    
    if (bytes_received < 0) {
        perror("recv");
        free(response);
        return NULL;
    }
    
    if (!found_end && total_size == 0) {
        return NULL;
    }
    
    return response;
}

void process_find_response(const char *response, int quiet_mode) {
    if (!quiet_mode) {
        printf("%s\n", response);
    } else {
        char *body_start = strstr(response, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
            printf("%s", body_start);
        }
    }
}

int process_get_response(const char *response, const char *download_dir, int quiet_mode) {
    char file_name[256] = {0};
    size_t file_size = 0;
    size_t encoded_size = 0;
    
    char *file_name_header = strstr(response, "File-Name:");
    if (file_name_header) {
        sscanf(file_name_header, "File-Name: %255[^\r\n]", file_name);
    } else {
        fprintf(stderr, "File-Name header not found in response\n");
        return -1;
    }
    char *file_size_header = strstr(response, "File-Size:");
    if (file_size_header) {
        sscanf(file_size_header, "File-Size: %zu", &file_size);
    }
    char *encoded_size_header = strstr(response, "Encoded-Size:");
    if (encoded_size_header) {
        sscanf(encoded_size_header, "Encoded-Size: %zu", &encoded_size);
    }
    if (!file_size || !encoded_size) {
        fprintf(stderr, "Invalid file size information in response\n");
        return -1;
    }
    char *headers_end = strstr(response, "\r\n\r\n");
    if (!headers_end) {
        fprintf(stderr, "Invalid response format: no header termination\n");
        return -1;
    }
    headers_end += 4; 
    if (!*headers_end) {
        fprintf(stderr, "Invalid response format: no content after headers\n");
        return -1;
    }
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", download_dir, file_name);
    if (create_parent_directories(full_path) != 0) {
        fprintf(stderr, "Failed to create directory for: %s\n", full_path);
        return -1;
    }
    if (!quiet_mode) {
        printf("Encoded data size: %zu bytes\n", strlen(headers_end));
        printf("First 20 chars of encoded data: ");
        for (int i = 0; i < 20 && headers_end[i]; i++) {
            printf("%c", headers_end[i]);
        }
        printf("\n");
    }
    size_t decoded_size;
    unsigned char *decoded_data = decode_base64(headers_end, &decoded_size);
    if (!decoded_data) {
        fprintf(stderr, "Failed to decode base64 data\n");
        return -1;
    }
    
    if (!quiet_mode) {
        printf("Saving file to: %s (%zu bytes)\n", full_path, decoded_size);
    }
    FILE *fp = fopen(full_path, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file for writing: %s\n", full_path);
        free(decoded_data);
        return -1;
    }
    
    size_t bytes_written = fwrite(decoded_data, 1, decoded_size, fp);
    fclose(fp);
    free(decoded_data);
    
    if (bytes_written != decoded_size) {
        fprintf(stderr, "Failed to write all data to file\n");
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s <server IP> <server port> <download directory> [quiet]\n", argv[0]);
        return 1;
    }
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char *download_dir = argv[3];
    int quiet_mode = (argc == 5 && strcmp(argv[4], "quiet") == 0);
    if (server_port <= 0 || server_port > 65535) { // not sure if I should make it between my own ports or any port? how does the TA check?
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        return 1;
    }
    if (!directory_exists(download_dir)) {
        fprintf(stderr, "Download directory does not exist: %s\n", download_dir);
        return 1;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }
    struct timeval tv;
    tv.tv_sec = CONNECTION_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", server_ip);
        close(sockfd);
        return 1;
    }
    if (!quiet_mode) {
        printf("Connecting to %s:%d...\n", server_ip, server_port);
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }
    
    if (!quiet_mode) {
        printf("Connected to server.\n");
    }
    
    char command[MAX_COMMAND_SIZE];
    int running = 1;
    
    while (running) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
            len--;
        }
        if (len == 0) {
            continue;
        }
        char send_buffer[MAX_COMMAND_SIZE + 4];
        snprintf(send_buffer, sizeof(send_buffer), "%s\r\n\r\n", command);
        
        if (send(sockfd, send_buffer, strlen(send_buffer), 0) < 0) {
            perror("Failed to send command");
            break;
        }
        int status_code = 0;
        char *response = receive_complete_response(sockfd, &status_code);
        
        if (!response) {
            fprintf(stderr, "Failed to receive response or connection closed by server\n");
            break;
        }
        if (strncmp(command, "HELO", 4) == 0) {
            if (!quiet_mode) {
                printf("Server response:\n%s\n", response);
            }
        } else if (strncmp(command, "FIND", 4) == 0) {
            process_find_response(response, quiet_mode);
        } else if (strncmp(command, "GET", 3) == 0) {
            if (status_code == 200) {
                process_get_response(response, download_dir, quiet_mode);
            } else if (!quiet_mode) {
                printf("Error response: %s\n", response);
            }
        } else if (strncmp(command, "END", 3) == 0) {
            if (!quiet_mode) {
                printf("Server response:\n%s\n", response);
            }
            running = 0;
        } else {
            if (!quiet_mode) {
                printf("Unknown command. Server response:\n%s\n", response);
            }
        }
        
        free(response);
    }
    close(sockfd);
    return 0;
}