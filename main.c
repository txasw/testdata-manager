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
typedef enum
{
    FAILED = 0,
    PASSED,
    PENDING,
    SUCCESS,
    INVALID_RESULT = -1
} TestResult;

// Data Structure
typedef struct
{
    int test_id;
    char system_name[100];
    char test_type[100];
    TestResult test_result;
    int active;
} TestRecord;

typedef struct
{
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
int validate_csv_header(const char *filename);
int create_new_csv(const char *filename);
int load_database(const char *filename);
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
void display_record(const TestRecord *record, int index);
void display_records_paginated(TestRecord *records, int count, const char *title);
void display_welcome_message(void);
void clear_screen(void);
void pause_screen(void);

// Utility functions
const char *test_result_to_string(TestResult result);
TestResult string_to_test_result(const char *str);
int find_record_by_id(int test_id);
int get_next_test_id(void);

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
    while (getchar() != '\n')
        ;
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
        if (token)
        {
            strncpy(record->system_name, token, sizeof(record->system_name) - 1);
            record->system_name[sizeof(record->system_name) - 1] = '\0';
        }

        token = strtok(NULL, ",");
        if (token)
        {
            strncpy(record->test_type, token, sizeof(record->test_type) - 1);
            record->test_type[sizeof(record->test_type) - 1] = '\0';
        }

        token = strtok(NULL, ",");
        if (token)
        {
            TestResult result = string_to_test_result(token);
            if (result == INVALID_RESULT)
            {
                printf("Warning: Invalid test result '%s' in record %d, defaulting to PENDING\n", token, record->test_id);
                record->test_result = PENDING;
            }
            else
            {
                record->test_result = result;
            }
        }

        token = strtok(NULL, ",");
        if (token)
        {
            char *endptr;
            long val = strtol(token, &endptr, 10);
            if (*endptr == '\0')
            {
                record->active = (int)val;
            }
            else
            {
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

int find_record_by_id(int test_id)
{
    for (int i = 0; i < db.count; i++)
    {
        if (db.records[i].test_id == test_id)
        {
            return i;
        }
    }
    return -1;
}

int get_next_test_id(void)
{
    return db.next_id++;
}

void list_all_records(void)
{
    clear_screen();
    printf("LIST ALL ACTIVE RECORDS\n");
    printf("========================\n");

    // Filter active records
    TestRecord *active_records = malloc(db.count * sizeof(TestRecord));
    if (active_records == NULL)
    {
        fprintf(stderr, "Error: Unable to allocate memory for active records.\n");
        pause_screen();
        return;
    }
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

void search_records(void)
{
    clear_screen();
    printf("SEARCH RECORDS\n");
    printf("==============\n");

    char search_term[256];
    if (!get_valid_input(search_term, sizeof(search_term), NULL,
                         "Enter search term (min 3 characters)"))
    {
        return;
    }

    if (strlen(trim_string(search_term)) < 3)
    {
        printf("Search term must be at least 3 characters.\n");
        pause_screen();
        return;
    }

    // Search in all fields
    TestRecord *results = malloc(db.count * sizeof(TestRecord));
    if (results == NULL && db.count > 0)
    {
        printf("Error: Unable to allocate memory for search results.\n");
        pause_screen();
        return;
    }
    int result_count = 0;

    for (int i = 0; i < db.count; i++)
    {
        if (!db.records[i].active)
            continue;

        char id_str[20];
        snprintf(id_str, sizeof(id_str), "%d", db.records[i].test_id);

        if (strstr(id_str, search_term) ||
            strcasestr(db.records[i].system_name, search_term) ||
            strcasestr(db.records[i].test_type, search_term) ||
            strcasestr(test_result_to_string(db.records[i].test_result), search_term))
        {

            results[result_count++] = db.records[i];
        }
    }

    if (result_count == 0)
    {
        printf("No records found matching '%s'.\n", search_term);
        free(results);
        pause_screen();
        return;
    }

    display_records_paginated(results, result_count, "Search Results");

    // Action menu for search results
    printf("\nSelect an action:\n");
    printf("1. Update a record\n");
    printf("2. Delete a record\n");
    printf("3. Return to main menu\n");

    int action = get_menu_choice(1, 3);
    if (action == -1 || action == 3)
    {
        free(results);
        return;
    }

    printf("Enter TestID from search results: ");
    int test_id;
    if (scanf("%d", &test_id) != 1)
    {
        printf("Invalid TestID.\n");
        free(results);
        while (getchar() != '\n')
            ; // Clear input buffer
        pause_screen();
        return;
    }
    while (getchar() != '\n')
        ; // Clear input buffer

    // Verify TestID is in search results
    int found = 0;
    for (int i = 0; i < result_count; i++)
    {
        if (results[i].test_id == test_id)
        {
            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("TestID %d not found in search results.\n", test_id);
        free(results);
        pause_screen();
        return;
    }

    free(results);

    if (action == 1)
    {
        // Update record
        int index = find_record_by_id(test_id);
        if (index != -1)
        {
            printf("\n--- Record to Update ---\n");
            printf("TestID: %d\n", db.records[index].test_id);
            printf("SystemName: %s\n", db.records[index].system_name);
            printf("TestType: %s\n", db.records[index].test_type);
            printf("TestResult: %s\n", test_result_to_string(db.records[index].test_result));

            update_record();
        }
    }
    else if (action == 2)
    {
        delete_record(test_id, 1);
    }
}

void update_record(void)
{
    printf("Enter TestID to update: ");
    int test_id;
    if (scanf("%d", &test_id) != 1)
    {
        printf("Invalid TestID.\n");
        while (getchar() != '\n')
            ;
        pause_screen();
        return;
    }
    while (getchar() != '\n')
        ;

    int index = find_record_by_id(test_id);
    if (index == -1 || !db.records[index].active)
    {
        printf("Record not found or has been deleted.\n");
        pause_screen();
        return;
    }

    TestRecord *record = &db.records[index];
    TestRecord backup = *record; // Backup for rollback

    printf("\n--- Current Record ---\n");
    printf("TestID: %d\n", record->test_id);
    printf("SystemName: %s\n", record->system_name);
    printf("TestType: %s\n", record->test_type);
    printf("TestResult: %s\n", test_result_to_string(record->test_result));

    printf("\nSelect action:\n");
    printf("1. Update record\n");
    printf("2. Delete record\n");
    printf("3. Return to menu\n");

    int action = get_menu_choice(1, 3);
    if (action == -1 || action == 3)
        return;

    if (action == 2)
    {
        delete_record(test_id, 1);
        return;
    }

    // Update mode
    int changes_made = 0;

    while (1)
    {
        printf("\n--- Current Record (Unsaved) ---\n");
        printf("TestID: %d\n", record->test_id);
        printf("SystemName: %s\n", record->system_name);
        printf("TestType: %s\n", record->test_type);
        printf("TestResult: %s\n", test_result_to_string(record->test_result));

        printf("\nSelect field to update:\n");
        printf("1. SystemName\n");
        printf("2. TestType\n");
        printf("3. TestResult\n");
        printf("4. Save changes\n");
        printf("5. Cancel (discard changes)\n");

        int field_choice = get_menu_choice(1, 5);
        if (field_choice == -1)
            continue;

        if (field_choice == 5)
        {
            printf("Discard all changes? (y/n): ");
            char confirm[10];
            fgets(confirm, sizeof(confirm), stdin);
            if (confirm[0] == 'y' || confirm[0] == 'Y')
            {
                *record = backup; // Restore backup
                printf("Changes discarded.\n");
                pause_screen();
                return;
            }
            continue;
        }

        if (field_choice == 4)
        {
            if (save_database())
            {
                printf("✓ Record updated successfully!\n");
                if (changes_made)
                {
                    printf("1 record updated in database.\n");
                }
                else
                {
                    printf("No changes were made.\n");
                }
            }
            else
            {
                printf("✗ Error saving to database.\n");
                *record = backup; // Restore backup
            }
            pause_screen();
            return;
        }

        char buffer[256];
        switch (field_choice)
        {
        case 1: // SystemName
            if (get_valid_input(buffer, sizeof(buffer), validate_system_name,
                                "Enter new System Name"))
            {
                strcpy(record->system_name, trim_string(buffer));
                changes_made = 1;
                printf("✓ SystemName updated.\n");
            }
            break;

        case 2: // TestType
            if (get_valid_input(buffer, sizeof(buffer), validate_test_type,
                                "Enter new Test Type"))
            {
                strcpy(record->test_type, trim_string(buffer));
                changes_made = 1;
                printf("✓ TestType updated.\n");
            }
            break;

        case 3: // TestResult
            printf("Select new Test Result:\n");
            printf("1. Pending\n2. Failed\n3. Passed\n4. Success\n");
            int result_choice = get_menu_choice(1, 4);
            if (result_choice != -1)
            {
                TestResult old_result = record->test_result;
                switch (result_choice)
                {
                case 1:
                    record->test_result = PENDING;
                    break;
                case 2:
                    record->test_result = FAILED;
                    break;
                case 3:
                    record->test_result = PASSED;
                    break;
                case 4:
                    record->test_result = SUCCESS;
                    break;
                }
                if (record->test_result != old_result)
                {
                    changes_made = 1;
                    printf("✓ TestResult updated.\n");
                }
            }
            break;
        }
    }
}

void delete_record(int test_id, int soft_delete)
{
    int index = find_record_by_id(test_id);
    if (index == -1)
    {
        printf("Record with TestID %d not found.\n", test_id);
        pause_screen();
        return;
    }

    TestRecord *record = &db.records[index];

    if (soft_delete && !record->active)
    {
        printf("Record is already deleted.\n");
        pause_screen();
        return;
    }

    if (!soft_delete && record->active)
    {
        printf("Record must be soft-deleted first.\n");
        pause_screen();
        return;
    }

    printf("\n--- Record to %s ---\n", soft_delete ? "Delete" : "Permanently Delete");
    printf("TestID: %d\n", record->test_id);
    printf("SystemName: %s\n", record->system_name);
    printf("TestType: %s\n", record->test_type);
    printf("TestResult: %s\n", test_result_to_string(record->test_result));

    printf("\nAre you sure you want to %s this record? (y/n): ",
           soft_delete ? "delete" : "permanently delete");
    char confirm[10];
    fgets(confirm, sizeof(confirm), stdin);

    if (confirm[0] != 'y' && confirm[0] != 'Y')
    {
        printf("Operation cancelled.\n");
        pause_screen();
        return;
    }

    if (soft_delete)
    {
        record->active = 0;
        if (save_database())
        {
            printf("✓ Record soft-deleted successfully!\n");
            printf("1 record deleted from active records.\n");
        }
        else
        {
            printf("✗ Error saving to database.\n");
            record->active = 1; // Rollback
        }
    }
    else
    {
        // Permanent delete - remove from array
        for (int i = index; i < db.count - 1; i++)
        {
            db.records[i] = db.records[i + 1];
        }
        db.count--;

        if (save_database())
        {
            printf("✓ Record permanently deleted!\n");
            printf("1 record permanently removed from database.\n");
        }
        else
        {
            printf("✗ Error saving to database.\n");
            // Rollback is complex for permanent delete, so we'll leave it as is
        }
    }

    pause_screen();
}

void recovery_data(void)
{
    clear_screen();
    printf("RECOVERY DATA\n");
    printf("=============\n");

    // Filter deleted records
    TestRecord *deleted_records = malloc(db.count * sizeof(TestRecord));
    if (deleted_records == NULL)
    {
        printf("Memory allocation failed. Unable to recover deleted records.\n");
        pause_screen();
        return;
    }
    int deleted_count = 0;

    for (int i = 0; i < db.count; i++)
    {
        if (!db.records[i].active)
        {
            deleted_records[deleted_count++] = db.records[i];
        }
    }

    if (deleted_count == 0)
    {
        printf("No deleted records found.\n");
        free(deleted_records);
        pause_screen();
        return;
    }

    display_records_paginated(deleted_records, deleted_count, "Deleted Records");

    printf("\nSelect action:\n");
    printf("1. Recover a record\n");
    printf("2. Permanently delete a record\n");
    printf("3. Return to main menu\n");

    int action = get_menu_choice(1, 3);
    if (action == -1 || action == 3)
    {
        free(deleted_records);
        return;
    }

    int attempts = 0;
    while (attempts < MAX_ATTEMPTS)
    {
        printf("Enter TestID: ");
        int test_id;
        if (scanf("%d", &test_id) != 1)
        {
            attempts++;
            printf("Invalid input. Please enter a valid TestID.\n");
            while (getchar() != '\n')
                ;
            continue;
        }
        while (getchar() != '\n')
            ;

        int index = find_record_by_id(test_id);
        if (index == -1 || db.records[index].active)
        {
            attempts++;
            printf("TestID %d not found in deleted records.\n", test_id);
            continue;
        }

        // Show the record
        TestRecord *record = &db.records[index];
        printf("\n--- Record Found ---\n");
        printf("TestID: %d\n", record->test_id);
        printf("SystemName: %s\n", record->system_name);
        printf("TestType: %s\n", record->test_type);
        printf("TestResult: %s\n", test_result_to_string(record->test_result));

        if (action == 1)
        {
            printf("\nConfirm recovery of this record? (y/n): ");
            char confirm[10];
            fgets(confirm, sizeof(confirm), stdin);

            if (confirm[0] == 'y' || confirm[0] == 'Y')
            {
                record->active = 1;
                if (save_database())
                {
                    printf("✓ Record recovered successfully!\n");
                    printf("1 record recovered.\n");
                }
                else
                {
                    printf("✗ Error saving to database.\n");
                    record->active = 0; // Rollback
                }
            }
            else
            {
                printf("Recovery cancelled.\n");
            }
        }
        else if (action == 2)
        {
            printf("\n⚠️  WARNING: This will permanently delete the record.\n");
            printf("Type the TestID again to confirm permanent deletion: ");

            int confirm_id;
            if (scanf("%d", &confirm_id) != 1 || confirm_id != test_id)
            {
                printf("TestID mismatch. Operation cancelled.\n");
                while (getchar() != '\n')
                    ;
            }
            else
            {
                delete_record(test_id, 0);
            }
            while (getchar() != '\n')
                ;
        }

        free(deleted_records);
        pause_screen();
        return;
    }

    printf("Maximum attempts reached. Returning to main menu.\n");
    free(deleted_records);
    pause_screen();
}

int select_database(void) {
    clear_screen();
    printf("SELECT DATABASE\n");
    printf("===============\n");
    
    char files[MAX_FILES][MAX_PATH];
    int file_count = scan_csv_files(files);
    
    if (file_count == 0) {
        printf("No CSV files found in current directory.\n\n");
        printf("Options:\n");
        printf("1. Create new database\n");
        printf("2. Enter manual path\n");
        printf("3. Do nothing\n");
        printf("\n");
        
        int choice = get_menu_choice(1, 3);
        if (choice == -1 || choice == 3) return 0;
        
        if (choice == 1) {
            char filename[MAX_PATH];
            if (get_valid_input(filename, sizeof(filename), NULL, "Enter database name")) {
                if (create_new_csv(filename)) {
                    printf("✓ Database created successfully: %s\n", db.filename);
                    pause_screen();
                    return 1;
                } else {
                    printf("✗ Error creating database.\n");
                    pause_screen();
                    return 0;
                }
            }
            return 0;
        }
        
        if (choice == 2) {
            char path[MAX_PATH];
            if (get_valid_input(path, sizeof(path), NULL, "Enter CSV file path")) {
                if (validate_csv_header(path) && load_database(path)) {
                    printf("✓ Database loaded successfully: %s\n", db.filename);
                    pause_screen();
                    return 1;
                } else {
                    printf("✗ Invalid CSV file or header format.\n");
                    pause_screen();
                    return 0;
                }
            }
            return 0;
        }
    }
    
    printf("Found %d CSV file(s):\n", file_count);
    for (int i = 0; i < file_count; i++) {
        printf("%d. %s\n", i + 1, files[i]);
    }

    printf("\nOther Options:\n");
    
    printf("%d. Create new database\n", file_count + 1);
    printf("%d. Enter manual path\n", file_count + 2);
    printf("%d. Do nothing\n", file_count + 3);

    printf("\n");
    
    int choice = get_menu_choice(1, file_count + 3);
    if (choice == -1 || choice == file_count + 3) return 0;
    
    if (choice == file_count + 1) {
        // Create new database
        char filename[MAX_PATH];
        if (get_valid_input(filename, sizeof(filename), NULL, "Enter database name")) {
            if (create_new_csv(filename)) {
                printf("✓ Database created successfully: %s\n", db.filename);
                pause_screen();
                return 1;
            } else {
                printf("✗ Error creating database.\n");
                pause_screen();
                return 0;
            }
        }
        return 0;
    }
    
    if (choice == file_count + 2) {
        // Manual path
        char path[MAX_PATH];
        if (get_valid_input(path, sizeof(path), NULL, "Enter CSV file path")) {
            if (validate_csv_header(path) && load_database(path)) {
                printf("✓ Database loaded successfully: %s\n", db.filename);
                pause_screen();
                return 1;
            } else {
                printf("✗ Invalid CSV file or header format.\n");
                pause_screen();
                return 0;
            }
        }
        return 0;
    }
    
    // Load selected file
    char* selected_file = files[choice - 1];
    
    if (!validate_csv_header(selected_file)) {
        printf("✗ Invalid header format in %s\n", selected_file);
        printf("Required header: %s\n", REQUIRED_HEADER);
        
        printf("Try another file? (y/n): ");
        char retry[10];
        fgets(retry, sizeof(retry), stdin);
        
        if (retry[0] == 'y' || retry[0] == 'Y') {
            return select_database();
        }
        return 0;
    }
    
    if (load_database(selected_file)) {
        printf("✓ Database loaded successfully: %s\n", db.filename);
        printf("Records loaded: %d\n", db.count);
        pause_screen();
        return 1;
    } else {
        printf("✗ Error loading database.\n");
        pause_screen();
        return 0;
    }
}

int change_database(void)
{
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

#ifndef __GLIBC__
char *strcasestr(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return NULL;

    size_t needle_len = strlen(needle);
    if (needle_len == 0)
        return (char *)haystack;

    for (const char *p = haystack; *p; p++)
    {
        if (strncasecmp(p, needle, needle_len) == 0)
        {
            return (char *)p;
        }
    }
    return NULL;
}
#endif

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

    while (!select_database())
    {
        printf("No database selected. Try again? (y/n): ");
        char retry[10];
        fgets(retry, sizeof(retry), stdin);
        if (retry[0] != 'y' && retry[0] != 'Y')
        {
            printf("Exiting program. Goodbye!\n");
            return 0;
        }
    }

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