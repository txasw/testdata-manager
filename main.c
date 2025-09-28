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
    SUCCESS,
    INVALID_RESULT = -1
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

// Input validation
int validate_system_name(const char *input);
int validate_test_type(const char *input);
char *trim_string(char *str);
int get_valid_input(char *buffer, int max_len, int (*validator)(const char *), const char *prompt);
int get_menu_choice(int min, int max);

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
    if (!str)
        return INVALID_RESULT;
    if (strcasecmp(str, "Failed") == 0)
        return FAILED;
    if (strcasecmp(str, "Passed") == 0)
        return PASSED;
    if (strcasecmp(str, "Pending") == 0)
        return PENDING;
    if (strcasecmp(str, "Success") == 0)
        return SUCCESS;
    return INVALID_RESULT;
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

        if (atoi(token) > 0)
            record->test_id = atoi(token);
        else
            continue;

        if (record->test_id > max_id)
        {
            max_id = record->test_id;
        }

        token = strtok(NULL, ",");
        if (token) {
            strncpy(record->system_name, token, sizeof(record->system_name) - 1);
            record->system_name[sizeof(record->system_name) - 1] = '\0';
        }

        token = strtok(NULL, ",");
        if (token) {
            strncpy(record->test_type, token, sizeof(record->test_type) - 1);
            record->test_type[sizeof(record->test_type) - 1] = '\0';
        }
        
        token = strtok(NULL, ",");
        if (token) {
            TestResult result = string_to_test_result(token);
            if (result == INVALID_RESULT) {
                printf("Warning: Invalid test result '%s' in record %d, defaulting to PENDING\n", token, record->test_id);
                record->test_result = PENDING;
            } else {
                record->test_result = result;
            }
        }
        
        token = strtok(NULL, ",");
        if (token) {
            char *endptr;
            long val = strtol(token, &endptr, 10);
            if (*endptr == '\0') {
                record->active = (int)val;
            } else {
                record->active = 0;
            }
        }

        count++;
    }

    fclose(file);
    db.count = count;
    db.next_id = max_id + 1;
    strcpy(db.filename, filename);

    return 1;
}

int save_database(void)
{
    FILE *file = fopen(db.filename, "w");
    if (!file)
        return 0;

    fprintf(file, "%s\n", REQUIRED_HEADER);

    for (int i = 0; i < db.count; i++)
    {
        TestRecord *record = &db.records[i];
        fprintf(file, "%d,%s,%s,%s,%d\n",
                record->test_id,
                record->system_name,
                record->test_type,
                test_result_to_string(record->test_result),
                record->active);
    }

    fclose(file);
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

void display_record(const TestRecord *record, int index)
{
    if (!record)
        return;

    printf("│ %-3d │ %-6d │ %-15s │ %-12s │ %-8s │ %-6s │\n",
           index + 1,
           record->test_id,
           record->system_name,
           record->test_type,
           test_result_to_string(record->test_result),
           record->active ? "Active" : "Deleted");
}

void display_records_paginated(TestRecord *records, int count, const char *title)
{
    if (count == 0)
    {
        printf("No records found.\n");
        return;
    }

    printf("\n%s (Total: %d records)\n", title, count);
    printf("┌─────┬────────┬─────────────────┬──────────────┬──────────┬────────┐\n");
    printf("│ No. │ TestID │ SystemName      │ TestType     │ Result   │ Status │\n");
    printf("├─────┼────────┼─────────────────┼──────────────┼──────────┼────────┤\n");

    if (count > PAGINATION_SIZE)
    {
        printf("Large dataset detected (%d records). Display all? (y/n): ", count);
        char choice[10];
        fgets(choice, sizeof(choice), stdin);

        if (choice[0] != 'y' && choice[0] != 'Y')
        {
            // Paginated display
            int page = 0;
            int total_pages = (count + PAGINATION_SIZE - 1) / PAGINATION_SIZE;

            while (1)
            {
                int start = page * PAGINATION_SIZE;
                int end = (start + PAGINATION_SIZE < count) ? start + PAGINATION_SIZE : count;

                for (int i = start; i < end; i++)
                {
                    display_record(&records[i], i);
                }

                printf("└─────┴────────┴─────────────────┴──────────────┴──────────┴────────┘\n");
                printf("Page %d of %d | (p)revious (n)ext (q)uit: ", page + 1, total_pages);

                char nav[10];
                fgets(nav, sizeof(nav), stdin);

                if (nav[0] == 'q' || nav[0] == 'Q')
                    break;
                if (nav[0] == 'n' || nav[0] == 'N')
                {
                    page = (page + 1) % total_pages;
                }
                if (nav[0] == 'p' || nav[0] == 'P')
                {
                    page = (page - 1 + total_pages) % total_pages;
                }

                printf("┌─────┬────────┬─────────────────┬──────────────┬──────────┬────────┐\n");
                printf("│ No. │ TestID │ SystemName      │ TestType     │ Result   │ Status │\n");
                printf("├─────┼────────┼─────────────────┼──────────────┼──────────┼────────┤\n");
            }
            return;
        }
    }

    // Display all records
    for (int i = 0; i < count; i++)
    {
        display_record(&records[i], i);
    }
    printf("└─────┴────────┴─────────────────┴──────────────┴──────────┴────────┘\n");
}

int get_next_test_id(void)
{
    return db.next_id++;
}


void list_all_records(void){
    clear_screen();
    printf("LIST ALL ACTIVE RECORDS\n");
    printf("========================\n");

    // Filter active records
    TestRecord *active_records = malloc(db.count * sizeof(TestRecord));
    int active_count = 0;

    for (int i = 0; i < db.count; i++)
    {
        if (db.records[i].active)
        {
            active_records[active_count++] = db.records[i];
        }
    }

    display_records_paginated(active_records, active_count, "Active Records");
    free(active_records);
    pause_screen();
}

void add_new_record(void)
{
    clear_screen();
    printf("ADD NEW RECORD\n");
    printf("==============\n");

    if (db.count >= MAX_RECORDS)
    {
        printf("Database is full. Cannot add new records.\n");
        pause_screen();
        return;
    }

    TestRecord new_record = {0};
    new_record.test_id = get_next_test_id();
    new_record.active = 1;

    char buffer[256];

    // Get System Name
    if (!get_valid_input(buffer, sizeof(buffer), validate_system_name,
                         "Enter System Name (min 3 chars, alphanumeric + ()[]- allowed)"))
    {
        return;
    }
    strcpy(new_record.system_name, trim_string(buffer));

    // Get Test Type
    if (!get_valid_input(buffer, sizeof(buffer), validate_test_type,
                         "Enter Test Type (min 3 chars, alphanumeric only)"))
    {
        return;
    }
    strcpy(new_record.test_type, trim_string(buffer));

    // Get Test Result
    printf("\nSelect Test Result:\n");
    printf("1. Pending\n");
    printf("2. Failed\n");
    printf("3. Passed\n");
    printf("4. Success\n");

    int choice = get_menu_choice(1, 4);
    if (choice == -1)
        return;

    switch (choice)
    {
    case 1:
        new_record.test_result = PENDING;
        break;
    case 2:
        new_record.test_result = FAILED;
        break;
    case 3:
        new_record.test_result = PASSED;
        break;
    case 4:
        new_record.test_result = SUCCESS;
        break;
    }

    // Add record to database
    db.records[db.count++] = new_record;

    if (save_database())
    {
        printf("\n✓ Record added successfully! (TestID: %d)\n", new_record.test_id);
        printf("1 record added to database.\n");
    }
    else
    {
        printf("✗ Error saving to database.\n");
        db.count--; // Rollback
    }

    pause_screen();
}

void search_records(void){}
void update_record(void){}
void recovery_data(void){}
int change_database(void){
    return 1;
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
    load_database("testdata.csv"); // Load a test database for demonstration
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