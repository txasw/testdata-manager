#include <stdio.h>
#include <stdlib.h>

int main() {
    // Open the CSV file
    FILE *file = fopen("system_testing_data.csv", "r");
    if (file == NULL) {
        perror("Failed to open CSV file");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END); // Move to end of file
    long file_size = ftell(file); // Get file size
    fseek(file, 0, SEEK_SET); // Move back to start

    // Allocate memory for file content (+1 for \0)
    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Failed to allocate memory");

        // Close the file before exiting in case of error
        fclose(file);
        return 1;
    }

    // Read file into buffer
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';

    // Check if read was successful
    if (bytes_read != file_size) {
        perror("Failed to read the entire file");

        // Clean up before exiting in case of error
        free(buffer);
        fclose(file);
        return 1;
    }

    // Testing output
    printf("%s\n", buffer);

    // Clean up and exit
    free(buffer);
    fclose(file);
    return 0;
}