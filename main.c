#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

char** get_csv_files(const char* dir_path, int* count) {
    DIR *d;
    struct dirent *dir;
    int files_found = 0;
    
    // Try to open the directory
    d = opendir(dir_path);
    if (d == NULL) {
        perror("Failed to open directory");
        *count = 0;
        return NULL;
    }

    // Read all entries in the directory to count .csv files
    while ((dir = readdir(d)) != NULL) {
        const char* filename = dir->d_name;
        size_t len = strlen(filename);

        // Check if the file has a .csv extension
        if (len >= 4 && strcmp(filename + len - 4, ".csv") == 0){
            files_found++;
        }
    }

    // Reset directory stream to the beginning
    rewinddir(d);

    // Allocate memory for the array of strings
    char** csv_files = NULL;
    if (files_found > 0) {
        csv_files = (char**)malloc((files_found + 1) * sizeof(char*));
        if (csv_files == NULL) {
            perror("Failed to allocate memory for file list");
            closedir(d);
            *count = 0;
            return NULL;
        }
    }
    // Populate the array with .csv filenames
    int i = 0;
    while ((dir = readdir(d)) != NULL) {
        const char* filename = dir->d_name;
        size_t len = strlen(filename);
        if (len >= 4 && strcmp(filename + len - 4, ".csv") == 0) {
            // +1 for null terminator
            csv_files[i] = (char*)malloc(len + 1);

            if (csv_files[i] == NULL) {
                perror("Failed to allocate memory for filename");
                for (int j = 0; j < i; j++) {
                    free(csv_files[j]);
                }
                free(csv_files);
                closedir(d);
                *count = 0;
                return NULL;
            }

            // Copy the filename into the allocated memory
            strcpy(csv_files[i], filename);
            i++;
        }
    }

    // Terminate the array with a NULL pointer
    if (csv_files != NULL) {
        csv_files[files_found] = NULL;
    }

    // Close the directory
    closedir(d);
    *count = files_found;
    return csv_files;
}

int main() {
    int count = 0;
    char** csv_files = get_csv_files(".", &count);
    if (csv_files == NULL) {
        printf("No .csv files found or an error occurred.\n");
    }else{
        printf("Found %d .csv files:\n", count);
        for (int i = 0; i < count; i++) {
            printf("%s\n", csv_files[i]);
            free(csv_files[i]);
        }
        free(csv_files);
    }
    return 0;
}