#include <stdio.h>
#include <dirent.h>

void get_csv_files(const char* dir_path) {
    DIR *d;
    struct dirent *dir;

    // Try to open the directory
    d = opendir(dir_path);
    if (d == NULL) {
        perror("Failed to open directory");
        return;
    }

    // Read and print all files in the directory
    while ((dir = readdir(d)) != NULL) {
        printf("File: %s\n", dir->d_name);
    }

    // Reset directory stream to the beginning
    rewinddir(d);

    // Close the directory
    closedir(d);
    return;
}

int main() {
    get_csv_files(".");

    return 0;
}