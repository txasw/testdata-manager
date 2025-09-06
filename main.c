#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char *data;
    size_t size;
} FileContent;

FileContent* read_file(const char *filename) {
    FileContent *content = (FileContent *)malloc(sizeof(FileContent));
    if (content == NULL) {
        perror("Failed to allocate memory for FileContent");
        return NULL;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        free(content);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);


    content->data = (char *)malloc(file_size + 1);
    if (content->data == NULL) {
        perror("Failed to allocate memory for file data");
        fclose(file);
        free(content);
        return NULL;
    }

    size_t bytes_read = fread(content->data, 1, file_size, file);
    content->data[bytes_read] = '\0';
    content->size = bytes_read;
    fclose(file);
    return content;
}

int main() {
    
    FileContent *csv = read_file("system_testing_data.csv");
    if (csv == NULL) {
        return EXIT_FAILURE;
    }
    printf("Size: %zu bytes\n", csv->size);
    printf("Content:\n%s\n", csv->data);
    free(csv->data);
    free(csv);
    return 0;
}