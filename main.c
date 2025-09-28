#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

// Constants
#define MAX_FILES 100
#define MAX_PATH 260
#define MAX_LINE 1024
#define MAX_RECORDS 10000
#define MAX_ATTEMPTS 3
#define PAGINATION_SIZE 50
#define MIN_NAME_LENGTH 3
#define REQUIRED_HEADER "TestID,SystemName,TestType,TestResult,Active"

// Test Result Options
typedef enum {
    FAILED = 0,
    PASSED,
    PENDING,
    SUCCESS
} TestResult;

// Data Structure
typedef struct {
    int test_id;
    char system_name[100];
    char test_type[100];
    TestResult test_result;
    int active;
} TestRecord;

typedef struct {
    TestRecord records[MAX_RECORDS];
    int count;
    char filename[MAX_PATH];
    int next_id;
} Database;

// Global database instance
Database db = {0};

// Function declarations
// File management
int scan_csv_files(char files[][MAX_PATH]);
int validate_csv_header(const char* filename);
int create_new_csv(const char* filename);
int load_database(const char* filename);
int save_database(void);

// CRUD operations
void list_all_records(void);
void add_new_record(void);
void search_records(void);
void update_record(void);
void delete_record(int test_id, int soft_delete);
void recovery_data(void);

// Display functions
void display_record(const TestRecord* record, int index);
void display_records_paginated(TestRecord* records, int count, const char* title);
void display_welcome_message(void);
void clear_screen(void);
void pause_screen(void);

// Utility functions
const char *test_result_to_string(TestResult result);
TestResult string_to_test_result(const char *str);

// Memory management
void cleanup_memory(void);

// Main menu functions
void show_main_menu(void);
int select_database(void);
int change_database(void);

// Implementation
#ifdef _WIN32
void clear_screen(void)
{
    system("cls");
}
#else
void clear_screen(void)
{
    system("clear");
}
#endif

void pause_screen(void)
{
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
}

char *trim_string(char *str)
{
    if (!str)
        return NULL;

    // Remove leading spaces
    while (isspace(*str))
        str++;

    // Remove trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;
    *(end + 1) = '\0';

    return str;
}

int validate_system_name(const char *input)
{
    if (!input)
        return 0;

    char *original = malloc(strlen(input) + 1);
    strcpy(original, input);
    char *trimmed = trim_string(original);

    int len = strlen(trimmed);
    if (len < MIN_NAME_LENGTH)
    {
        free(original);
        return 0;
    }

    for (int i = 0; i < len; i++)
    {
        char c = trimmed[i];
        if (!isalnum(c) && c != '(' && c != ')' && c != '[' && c != ']' &&
            c != '-' && c != '_' && c != '.' && c != ' ')
        {
            free(original);
            return 0;
        }
    }

    free(original);
    return 1;
}

int validate_test_type(const char *input)
{
    if (!input)
        return 0;

    char *original = malloc(strlen(input) + 1);
    strcpy(original, input);
    char *trimmed = trim_string(original);

    int len = strlen(trimmed);
    if (len < MIN_NAME_LENGTH)
    {
        free(original);
        return 0;
    }

    for (int i = 0; i < len; i++)
    {
        if (!isalnum(trimmed[i]))
        {
            free(original);
            return 0;
        }
    }

    free(original);
    return 1;
}

int get_valid_input(char *buffer, int max_len, int (*validator)(const char *), const char *prompt)
{
    int attempts = 0;

    while (attempts < MAX_ATTEMPTS)
    {
        printf("%s: ", prompt);
        if (!fgets(buffer, max_len, stdin))
        {
            attempts++;
            printf("Error reading input. Please try again.\n");
            continue;
        }

        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(trim_string(buffer)) == 0)
        {
            attempts++;
            printf("Empty input detected. Please try again.\n");
            continue;
        }

        if (validator && !validator(buffer))
        {
            attempts++;
            printf("Invalid input format. Please try again.\n");
            continue;
        }

        return 1;
    }

    printf("Maximum attempts reached. Operation cancelled.\n");
    return 0;
}

int get_menu_choice(int min, int max)
{
    char input[10];
    int choice;
    int attempts = 0;

    while (attempts < MAX_ATTEMPTS)
    {
        printf("Enter your choice (%d-%d): ", min, max);
        if (!fgets(input, sizeof(input), stdin))
        {
            attempts++;
            continue;
        }

        input[strcspn(input, "\n")] = '\0';

        if (sscanf(input, "%d", &choice) == 1 && choice >= min && choice <= max)
        {
            return choice;
        }

        attempts++;
        printf("Invalid choice. Please enter a number between %d and %d.\n", min, max);
    }

    printf("Maximum attempts reached. Returning to main menu.\n");
    return -1;
}

#ifdef _WIN32
int scan_csv_files(char files[][MAX_PATH])
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    int count = 0;

    hFind = FindFirstFile("*.csv", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    do
    {
        if (count < MAX_FILES)
        {
            strcpy(files[count], findFileData.cFileName);
            count++;
        }
    } while (FindNextFile(hFind, &findFileData) != 0 && count < MAX_FILES);

    FindClose(hFind);
    return count;
}
#else
int scan_csv_files(char files[][MAX_PATH])
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(".");
    if (!dir)
        return 0;

    while ((entry = readdir(dir)) != NULL && count < MAX_FILES)
    {
        char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".csv") == 0)
        {
            strcpy(files[count], entry->d_name);
            count++;
        }
    }

    closedir(dir);
    return count;
}
#endif

int validate_csv_header(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
        return 0;

    char line[MAX_LINE];
    if (!fgets(line, sizeof(line), file))
    {
        fclose(file);
        return 0;
    }

    line[strcspn(line, "\n\r")] = '\0';

    fclose(file);
    return strcmp(line, REQUIRED_HEADER) == 0;
}

int create_new_csv(const char *filename)
{
    char full_filename[MAX_PATH];

    // Add .csv extension if not present
    if (strstr(filename, ".csv") == NULL)
    {
        snprintf(full_filename, sizeof(full_filename), "%s.csv", filename);
    }
    else
    {
        strcpy(full_filename, filename);
    }

    // check if file exists
#ifdef _WIN32
    if (_access(full_filename, 0) != -1)
#else
    if (access(full_filename, F_OK) != -1)
#endif
    {
        printf("File '%s' already exists. Choose a different name.\n", full_filename);
        return 0;
    }

    FILE *file = fopen(full_filename, "w");
    if (!file)
        return 0;

    fprintf(file, "%s\n", REQUIRED_HEADER);
    fclose(file);

    strcpy(db.filename, full_filename);
    db.count = 0;
    db.next_id = 1;

    return 1;
}

const char *test_result_to_string(TestResult result)
{
    switch (result)
    {
    case FAILED:
        return "Failed";
    case PASSED:
        return "Passed";
    case PENDING:
        return "Pending";
    case SUCCESS:
        return "Success";
    default:
        return "Unknown";
    }
}

TestResult string_to_test_result(const char *str)
{
    if (strcasecmp(str, "Failed") == 0)
        return FAILED;
    if (strcasecmp(str, "Passed") == 0)
        return PASSED;
    if (strcasecmp(str, "Pending") == 0)
        return PENDING;
    if (strcasecmp(str, "Success") == 0)
        return SUCCESS;
    return PENDING;
}

int load_database(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
        return 0;

    char line[MAX_LINE];
    int count = 0;
    int max_id = 0;

    // Skip header
    if (!fgets(line, sizeof(line), file))
    {
        fclose(file);
        return 0;
    }

    memset(&db, 0, sizeof(db));

    // Read records
    while (fgets(line, sizeof(line), file) && count < MAX_RECORDS)
    {
        line[strcspn(line, "\n\r")] = '\0';

        char *token = strtok(line, ",");
        if (!token)
            continue;

        TestRecord *record = &db.records[count];
        record->test_id = atoi(token);

        if (record->test_id > max_id)
        {
            max_id = record->test_id;
        }

        token = strtok(NULL, ",");
        if (token)
            strcpy(record->system_name, token);

        token = strtok(NULL, ",");
        if (token)
            strcpy(record->test_type, token);

        token = strtok(NULL, ",");
        if (token)
            record->test_result = string_to_test_result(token);

        token = strtok(NULL, ",");
        if (token)
            record->active = atoi(token);

        count++;
    }

    fclose(file);
    db.count = count;
    db.next_id = max_id + 1;
    strcpy(db.filename, filename);

    return 1;
}


void list_all_records(void){}
void add_new_record(void){}
void search_records(void){}
void update_record(void){}
void recovery_data(void){}
int change_database(void){
    return 1;
}

void display_welcome_message(void)
{
    clear_screen();
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    SYSTEM TESTING DATA MANAGER               ║\n");
    printf("║                     ระบบจัดการข้อมูลการทดสอบระบบ                ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Current Database: %-42s ║\n", "None");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

void show_main_menu(void)
{
    display_welcome_message();

    printf("Main Menu:\n");
    printf("1. List all records\n");
    printf("2. Add new record\n");
    printf("3. Search records\n");
    printf("4. Update record\n");
    printf("5. Recovery data\n");
    printf("6. Change database\n");
    printf("7. Run tests\n");
    printf("8. Exit program\n");
}

void cleanup_memory(void)
{
    // In this implementation, we use static arrays, so no dynamic cleanup needed
    // But we could add any necessary cleanup here
    printf("Memory cleanup completed.\n");
}

// Main function
int main(void)
{

    // test load database
    if (!load_database("testdata.csv"))
    {
        printf("Failed to load database 'testdata.csv'.\n");
    }
    else
    {
        printf("Database 'testdata.csv' loaded successfully with %d records.\n", db.count);
    }
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                 SYSTEM TESTING DATA MANAGER                  ║\n");
    printf("║                  ระบบจัดการข้อมูลกรทดสอบระบบ                    ║\n");
    printf("║                                                              ║\n");
    printf("║                 Welcome to the application!                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    pause_screen();

    // Main application loop
    while (1)
    {
        show_main_menu();

        int choice = get_menu_choice(1, 8);
        if (choice == -1)
        {

            pause_screen();
            continue;
        }

        switch (choice)
        {
        case 1:
            list_all_records();
            break;
        case 2:
            add_new_record();
            break;
        case 3:
            search_records();
            break;
        case 4:
            update_record();
            break;
        case 5:
            recovery_data();
            break;
        case 6:
            change_database();
            break;
        case 7:
            printf("\nSelect test type:\n");
            printf("1. Unit tests\n");
            printf("2. End-to-end tests\n");
            printf("3. Return to main menu\n");

            int test_choice = get_menu_choice(1, 3);
            if (test_choice == 1)
            {
                printf("Running unit tests...\n");
                pause_screen();
            }
            else if (test_choice == 2)
            {
                printf("Running end-to-end tests...\n");
                pause_screen();
            }
            break;
        case 8:
            printf("Are you sure you want to exit? (y/n): ");
            char confirm[10];
            fgets(confirm, sizeof(confirm), stdin);
            if (confirm[0] == 'y' || confirm[0] == 'Y')
            {
                cleanup_memory();
                printf("Thank you for using System Testing Data Manager!\n");
                return 0;
            }
            break;
        }
    }

    return 0;
}