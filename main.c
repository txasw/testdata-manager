#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


char **get_csv_files(const char *dir_path, int *count)
{
    DIR *d;
    struct dirent *dir;
    int files_found = 0;

    // Try to open the directory
    d = opendir(dir_path);
    if (d == NULL)
    {
        perror("Failed to open directory");
        *count = 0;
        return NULL;
    }

    // Read all entries in the directory to count .csv files
    while ((dir = readdir(d)) != NULL)
    {
        const char *filename = dir->d_name;
        size_t len = strlen(filename);

        // Check if the file has a .csv extension
        if (len >= 4 && strcmp(filename + len - 4, ".csv") == 0)
        {
            files_found++;
        }
    }

    // Reset directory stream to the beginning
    rewinddir(d);

    // Allocate memory for the array of strings
    char **csv_files = NULL;
    if (files_found > 0)
    {
        csv_files = (char **)malloc((files_found + 1) * sizeof(char *));
        if (csv_files == NULL)
        {
            perror("Failed to allocate memory for file list");
            closedir(d);
            *count = 0;
            return NULL;
        }
    }
    // Populate the array with .csv filenames
    int i = 0;
    while ((dir = readdir(d)) != NULL)
    {
        const char *filename = dir->d_name;
        size_t len = strlen(filename);
        if (len >= 4 && strcmp(filename + len - 4, ".csv") == 0)
        {
            // +1 for null terminator
            csv_files[i] = (char *)malloc(len + 1);

            if (csv_files[i] == NULL)
            {
                perror("Failed to allocate memory for filename");
                for (int j = 0; j < i; j++)
                {
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
    if (csv_files != NULL)
    {
        csv_files[files_found] = NULL;
    }

    // Close the directory
    closedir(d);
    *count = files_found;
    return csv_files;
}

void free_csv_files(char **files)
{
    if (files == NULL)
        return;
    for (int i = 0; files[i] != NULL; i++)
    {
        free(files[i]);
    }
    free(files);
}

int main()
{
    char database_path[256];
    printf("Greeting! Welcome to the 'Testing Data Manager Program'\n");
    printf("Loading all database files (.csv)...\n");

    int count = 0;
    char **csv_files = get_csv_files(".", &count);

    // csv_files = NULL; // For testing no files found scenario

    if (csv_files == NULL)
    {
        // TODO: Make this into a function to handle user input
        char input[256];
        while (1)
        {
            printf("Not found any database files, Would you like to create a new one? (Y/N) : ");
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL)
            {
                continue;
            }

            input[strcspn(input, "\n")] = 0;
            if (strlen(input) == 1)
            {
                char choice = tolower(input[0]);
                if (choice == 'y')
                {
                    // TODO: Implement create new database file functionality
                    // TODO: Let user specify the name of the new database file
                    printf("Feature to create a new database file is not yet implemented.\n");
                    break;
                }
                else if (choice == 'n')
                {
                    // TODO: Implement enter path manually functionality
                    printf("Feature to enter path manually is not yet implemented.\n");
                    break;
                }
            }
        }
    }
    else
    {
        printf("There are %d CSV files found in this directory!\n", count);
        printf("=============================================\n");
        printf("AVAILABLE DATABASE FILES IN CURRENT DIRECTORY\n");
        printf("=============================================\n");

        for (int i = 0; i < count; i++)
        {
            printf("[%d] %s\n", i, csv_files[i]);
        }
        printf("\nOR\n\n");
        printf("[N] Create a New database file\n");
        printf("[M] Enter path manually\n");
        printf("[X] Exit program\n");
        printf("\n");

        // TODO: Make this into a function to handle user input
        char input[256];
        while (1)
        {
            printf("Please select database file (");
            if (count > 1)
            {
                printf("0-%d", count - 1);
            }
            else
            {
                printf("0");
            }
            printf("/N/M/X): ");

            // Ensure prompt is shown before input
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL)
            {
                continue;
            }

            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "X") == 0 || strcmp(input, "x") == 0 || strcmp(input, "Q") == 0 || strcmp(input, "q") == 0)
            {
                printf("Exiting program. Goodbye!\n");
                break;
            }
            else if (strcmp(input, "N") == 0 || strcmp(input, "n") == 0)
            {
                printf("Feature to create a new database file is not yet implemented.\n");
                break;
            }
            else if (strcmp(input, "M") == 0 || strcmp(input, "m") == 0)
            {
                printf("Entering path manually...\n");
                break;
            }
            else
            {
                int selection;
                if (sscanf(input, "%d", &selection) == 1)
                {
                    if (selection >= 0 && selection < count)
                    {
                        strcpy(database_path, csv_files[selection]);
                        break;
                    }
                }
                printf("Invalid input. Please try again.\n");
            }
        }
    }

    // TODO: Check if database is in valid format
    printf("=============================================\n");
    printf("WELCOME TO THE TESTING DATA MANAGER\n");
    printf("DATABASE: %s\n", database_path);
    printf("=============================================\n");

    free_csv_files(csv_files);

    return 0;
}