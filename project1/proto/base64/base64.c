#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s encode <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "encode") != 0) {
        fprintf(stderr, "Error: Only 'encode' operation is supported.\n");
        return EXIT_FAILURE;
    }

    char command[1024];
    snprintf(command, sizeof(command), "base64 %s", argv[2]);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error executing base64 command");
        return EXIT_FAILURE;
    }

    /* Read the output from the base64 command into a dynamically allocated buffer */
    char *encoded = NULL;
    size_t encoded_size = 0;
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        char *temp = realloc(encoded, encoded_size + bytesRead + 1);
        if (!temp) {
            fprintf(stderr, "Memory allocation error\n");
            free(encoded);
            pclose(fp);
            return EXIT_FAILURE;
        }
        encoded = temp;
        memcpy(encoded + encoded_size, buffer, bytesRead);
        encoded_size += bytesRead;
        encoded[encoded_size] = '\0';
    }
    pclose(fp);

    printf("Content-Size: %zu\n", encoded_size);

    char output_filename[1024];
    snprintf(output_filename, sizeof(output_filename), "%s.encoded", argv[2]);
    FILE *outfp = fopen(output_filename, "w");
    if (!outfp) {
        perror("Error opening output file");
        free(encoded);
        return EXIT_FAILURE;
    }
    fwrite(encoded, 1, encoded_size, outfp);
    fclose(outfp);

    free(encoded);
    return EXIT_SUCCESS;
}
