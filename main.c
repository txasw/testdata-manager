#include <stdio.h>
#include <dirent.h>

void get_csv_files(const char* dir_path, int* count) {
    DIR *d;
    struct dirent *dir;
    int files_found = 0;

    // Try to open the directory
    d = opendir(dir_path);
    if (d == NULL) {
        perror("Failed to open directory");
        *count = 0;
        return;
    }

    // Read and print all files in the directory
    while ((dir = readdir(d)) != NULL) {
        files_found++;
        printf("File: %s\n", dir->d_name);
    }

    // Reset directory stream to the beginning
    rewinddir(d);

    // Close the directory
    closedir(d);
    *count = files_found;
    return;
}

int main() {
    int count = 0;
    get_csv_files(".", &count);
    printf("Total files found: %d\n", count);

    return 0;
}