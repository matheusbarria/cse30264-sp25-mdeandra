#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Global counter for file enumeration
static int file_counter = 1;

// Function to process a file and print its details
void process_file(const char* base_path, const char* curr_path, const char* filename) {
    char full_path[4096];
    char relative_path[4096];
    struct stat st;
    
    snprintf(full_path, sizeof(full_path), "%s/%s", curr_path, filename);
    if (stat(full_path, &st) == 0) {
        if (strlen(curr_path) > strlen(base_path)) {
            // We're in a subdirectory
            snprintf(relative_path, sizeof(relative_path), "%s/%s",
                    curr_path + strlen(base_path) + 1, filename);
        } else {
            // We're in the base directory
            strncpy(relative_path, filename, sizeof(relative_path) - 1);
        }
        printf("%d;%s;%ld\n", file_counter++, relative_path, st.st_size);
        
    }
}

// Recursive function to traverse directory
void traverse_directory(const char* base_path, const char* curr_path) {
    DIR* dir;
    struct dirent* entry;
    dir = opendir(curr_path);
    if (!dir) {
        fprintf(stderr, "Error opening directory %s: %s\n", 
                curr_path, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[4096];
        struct stat st;
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Construct full path
        snprintf(path, sizeof(path), "%s/%s", curr_path, entry->d_name);
        
        // Get file/directory information
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // if entry is directory
                traverse_directory(base_path, path);
            } else if (S_ISREG(st.st_mode)) {
                // regular file
                process_file(base_path, curr_path, entry->d_name);
            }
        }
    }
    
    closedir(dir);
}

int main(int argc, char* argv[]) {
    struct stat st;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }
    if (stat(argv[1], &st) != 0) {
        fprintf(stderr, "Error: Cannot access %s: %s\n", 
                argv[1], strerror(errno));
        return 1;
    }
    // Check if it's actually a directory
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a directory\n", argv[1]);
        return 1;
    }
    traverse_directory(argv[1], argv[1]);
    
    return 0;
}