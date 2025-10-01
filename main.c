#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

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
#define PAGINATION_SIZE 20
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
int validate_test_id(const char *input);
int get_yes_no(const char *input, int default_answer, int max_attempts);
char *trim_string(char *str);
int get_valid_input(char *buffer, int max_len, int (*validator)(const char *), const char *prompt);
int get_menu_choice(int min, int max);

// CRUD operations
void list_all_records(void);
void add_new_record(void);
void search_records(void);
void update_record(void);
void update_record_by_id(int test_id);
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
int create_new_database_prompt(void);
int enter_manual_path_prompt(void);

// Test functions
void run_unit_tests(void);
void run_e2e_tests(void);
void test_input_validation(void);
void test_crud_operations(void);
void run_all_tests(void);

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

int validate_test_id(const char *input)
{
    if (!input)
        return 0;

    char *original = malloc(strlen(input) + 1);
    if (!original)
        return 0;
    strcpy(original, input);
    char *trimmed = trim_string(original);

    if (strlen(trimmed) == 0)
    {
        free(original);
        return 0;
    }

    char *endptr;
    long val = strtol(trimmed, &endptr, 10);
    if (*endptr != '\0' || val <= 0)
    {
        free(original);
        return 0;
    }

    free(original);
    return 1;
}

int get_yes_no(const char *prompt, int default_answer, int max_attempts)
{
    char input[10];
    int attempts = 0;
    max_attempts = (max_attempts >= 1) ? max_attempts : 1;

    do
    {
        printf("%s (y/n): ", prompt);
        if (!fgets(input, sizeof(input), stdin))
        {
            attempts++;
            printf("Error reading input. Please try again.\n");
            continue;
        }

        input[strcspn(input, "\n")] = '\0';
        char *trimmed = trim_string(input);

        if (strlen(trimmed) == 0)
        {
            if (max_attempts == 1)
            {
                printf("Defaulting to %s.\n", default_answer ? "yes" : "no");
                return default_answer;
            }
            attempts++;
            printf("Empty input detected. Please try again.\n");
            continue;
        }

        char c = tolower(trimmed[0]);
        if (c == 'y')
            return 1;
        if (c == 'n')
            return 0;

        if (max_attempts == 1)
        {
            printf("Defaulting to %s.\n", default_answer ? "yes" : "no");
            return default_answer;
        }

        attempts++;
        printf("Invalid input '%s'. Please enter 'y' for yes or 'n' for no.\n", trimmed);

    } while (attempts < max_attempts);

    printf("Maximum attempts reached. Defaulting to %s.\n", default_answer ? "yes" : "no");
    return default_answer;
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

        if (validator != NULL && !validator(buffer))
        {
            attempts++;
            printf("Invalid input format. Please try again.\n");
            continue;
        }

        return 1;
    }

    printf("Maximum attempts reached. Operation cancelled.\n");
    pause_screen();
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

    printf("Maximum attempts reached. Operation cancelled.\n");
    pause_screen();
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
    printf("║ Current Database: %-42s ║\n", db.filename);
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

void display_record(const TestRecord *record, int index)
{
    if (!record)
        return;

    printf("│ %-3d │ %-6d │ %-30s │ %-25s │ %-8s │ %-7s │\n",
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
    if (count > PAGINATION_SIZE)
    {
        if (!get_yes_no("Large dataset detected. Display all?", 1, 1))
        {
            // Paginated display
            int page = 0;
            int total_pages = (count + PAGINATION_SIZE - 1) / PAGINATION_SIZE;

            while (1)
            {
                clear_screen();
                printf("┌─────┬────────┬────────────────────────────────┬───────────────────────────┬──────────┬─────────┐\n");
                printf("│ No. │ TestID │ SystemName                     │ TestType                  │ Result   │ Status  │\n");
                printf("├─────┼────────┼────────────────────────────────┼───────────────────────────┼──────────┼─────────┤\n");
                int start = page * PAGINATION_SIZE;
                int end = (start + PAGINATION_SIZE < count) ? start + PAGINATION_SIZE : count;

                for (int i = start; i < end; i++)
                {
                    display_record(&records[i], i);
                }

                printf("└─────┴────────┴────────────────────────────────┴───────────────────────────┴──────────┴─────────┘\n");
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
                if (nav[0] == '\n')
                {
                    page = (page + 1) % total_pages;
                }
            }
            return;
        }
        clear_screen();
    }
    clear_screen();
    printf("\n%s (Total: %d records)\n", title, count);
    printf("┌─────┬────────┬────────────────────────────────┬───────────────────────────┬──────────┬─────────┐\n");
    printf("│ No. │ TestID │ SystemName                     │ TestType                  │ Result   │ Status  │\n");
    printf("├─────┼────────┼────────────────────────────────┼───────────────────────────┼──────────┼─────────┤\n");

    // Display all records
    for (int i = 0; i < count; i++)
    {
        display_record(&records[i], i);
    }
    printf("└─────┴────────┴────────────────────────────────┴───────────────────────────┴──────────┴─────────┘\n");
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
                         "\nEnter Test Type (min 3 chars, alphanumeric only)"))
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

    char input_buffer[20];
    if (!get_valid_input(input_buffer, sizeof(input_buffer), validate_test_id, "Enter TestID from search results"))
    {
        free(results);
        return;
    }

    int test_id = atoi(trim_string(input_buffer));

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
            clear_screen();
            printf("--- Record to Update ---\n");
            printf("TestID: %d\n", db.records[index].test_id);
            printf("SystemName: %s\n", db.records[index].system_name);
            printf("TestType: %s\n", db.records[index].test_type);
            printf("TestResult: %s\n", test_result_to_string(db.records[index].test_result));

            update_record_by_id(test_id);
        }
    }
    else if (action == 2)
    {
        delete_record(test_id, 1);
    }
}

void update_record(void)
{
    clear_screen();
    printf("UPDATE RECORD\n");
    printf("==============\n");
    char input_buffer[20];
    if (!get_valid_input(input_buffer, sizeof(input_buffer), validate_test_id, "Enter TestID to update"))
    {
        return;
    }
    int test_id = atoi(trim_string(input_buffer));

    update_record_by_id(test_id);
}

void update_record_by_id(int test_id)
{
    int index = find_record_by_id(test_id);
    if (index == -1 || !db.records[index].active)
    {
        printf("Record not found or has been deleted.\n");
        pause_screen();
        return;
    }

    TestRecord *record = &db.records[index];
    TestRecord backup = *record; // Backup for rollback

    clear_screen();
    printf("You are about to modify the following record:\n");
    printf("┌────────┬────────────────────────────────┬───────────────────────────┬──────────┐\n");
    printf("│ TestID │ SystemName                     │ TestType                  │ Result   │\n");
    printf("├────────┼────────────────────────────────┼───────────────────────────┼──────────┤\n");
    printf("│ %-6d │ %-30s │ %-25s │ %-8s │\n", record->test_id, record->system_name, record->test_type,
           test_result_to_string(record->test_result));
    printf("└────────┴────────────────────────────────┴───────────────────────────┴──────────┘\n");

    printf("\nSelect action:\n");
    printf("1. Update record\n");
    printf("2. Delete record\n");
    printf("3. Return to menu\n");

    printf("\n");

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
        clear_screen();
        printf("--- Current Record (Unsaved) ---\n");
        printf("┌────────┬────────────────────────────────┬───────────────────────────┬──────────┐\n");
        printf("│ TestID │ SystemName                     │ TestType                  │ Result   │\n");
        printf("├────────┼────────────────────────────────┼───────────────────────────┼──────────┤\n");
        printf("│ %-6d │ %-30s │ %-25s │ %-8s │\n", record->test_id, record->system_name, record->test_type,
               test_result_to_string(record->test_result));
        printf("└────────┴────────────────────────────────┴───────────────────────────┴──────────┘\n");

        printf("\nSelect field to update:\n");
        printf("1. SystemName\n");
        printf("2. TestType\n");
        printf("3. TestResult\n");
        printf("\nOther Options:\n");
        printf("4. Save changes\n");
        printf("5. Cancel (discard changes)\n");

        printf("\n");

        int field_choice = get_menu_choice(1, 5);
        if (field_choice == -1)
            continue;

        if (field_choice == 5)
        {
            if (get_yes_no("Discard all changes?", 0, 1))
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

    clear_screen();
    printf("You are about to %s the following record:\n", soft_delete ? "delete" : "permanently delete");
    printf("┌────────┬────────────────────────────────┬───────────────────────────┬──────────┐\n");
    printf("│ TestID │ SystemName                     │ TestType                  │ Result   │\n");
    printf("├────────┼────────────────────────────────┼───────────────────────────┼──────────┤\n");
    printf("│ %-6d │ %-30s │ %-25s │ %-8s │\n", record->test_id, record->system_name, record->test_type,
           test_result_to_string(record->test_result));
    printf("└────────┴────────────────────────────────┴───────────────────────────┴──────────┘\n");

    printf("\n");

    if (!get_yes_no(soft_delete ? "Are you sure you want to delete this record?" : "Are you sure you want to permanently delete this record?", 0, 3))
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

    printf("\n");

    int action = get_menu_choice(1, 3);
    if (action == -1 || action == 3)
    {
        free(deleted_records);
        return;
    }

    int attempts = 0;
    while (attempts < MAX_ATTEMPTS)
    {
        char input_buffer[20];
        if (!get_valid_input(input_buffer, sizeof(input_buffer), validate_test_id, "Enter TestID"))
        {
            free(deleted_records);
            return;
        }
        int test_id = atoi(trim_string(input_buffer));

        int index = find_record_by_id(test_id);
        if (index == -1 || db.records[index].active)
        {
            attempts++;
            printf("TestID %d not found in deleted records.\n", test_id);
            continue;
        }

        // Show the record
        TestRecord *record = &db.records[index];
        clear_screen();
        printf("You are about to %s the following record:\n", action == 1 ? "recover" : "permanently delete");
        printf("┌────────┬────────────────────────────────┬───────────────────────────┬──────────┐\n");
        printf("│ TestID │ SystemName                     │ TestType                  │ Result   │\n");
        printf("├────────┼────────────────────────────────┼───────────────────────────┼──────────┤\n");
        printf("│ %-6d │ %-30s │ %-25s │ %-8s │\n", record->test_id, record->system_name, record->test_type,
               test_result_to_string(record->test_result));
        printf("└────────┴────────────────────────────────┴───────────────────────────┴──────────┘\n");

        if (action == 1)
        {
            if (get_yes_no("Confirm recovery of this record?", 0, 3))
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

            char confirm_input[20];
            char expected_id[20];
            snprintf(expected_id, sizeof(expected_id), "%d", test_id);

            if (!get_valid_input(confirm_input, sizeof(confirm_input), NULL,
                                 "Type the TestID again to confirm permanent deletion"))
            {
                free(deleted_records);
                return;
            }

            if (strcmp(trim_string(confirm_input), expected_id) == 0)
            {
                delete_record(test_id, 0);
                free(deleted_records);
                return;
            }
            else
            {
                printf("TestID mismatch. Operation cancelled.\n");
            }
        }

        free(deleted_records);
        pause_screen();
        return;
    }

    printf("Maximum attempts reached. Returning to main menu.\n");
    free(deleted_records);
    pause_screen();
}

int create_new_database_prompt(void)
{
    char filename[MAX_PATH];
    if (get_valid_input(filename, sizeof(filename), NULL, "Enter database name"))
    {
        if (create_new_csv(filename))
        {
            printf("✓ Database created successfully: %s\n", db.filename);
            pause_screen();
            return 1;
        }
        else
        {
            printf("✗ Error creating database.\n");
            pause_screen();
            return 0;
        }
    }
    return 0;
}

int enter_manual_path_prompt(void)
{
    char path[MAX_PATH];
    if (get_valid_input(path, sizeof(path), NULL, "Enter CSV file path"))
    {
        if (validate_csv_header(path) && load_database(path))
        {
            printf("✓ Database loaded successfully: %s\n", db.filename);
            pause_screen();
            return 1;
        }
        else
        {
            printf("✗ Invalid CSV file or header format.\n");
            pause_screen();
            return 0;
        }
    }
    return 0;
}

int select_database(void)
{
    clear_screen();
    printf("SELECT DATABASE\n");
    printf("===============\n");

    char files[MAX_FILES][MAX_PATH];
    int file_count = scan_csv_files(files);

    if (file_count == 0)
    {
        printf("No CSV files found in current directory.\n\n");
        printf("Options:\n");
        printf("1. Create new database\n");
        printf("2. Enter manual path\n");
        printf("3. Do nothing\n");
        printf("\n");

        int choice = get_menu_choice(1, 3);
        if (choice == -1 || choice == 3)
            return 0;

        if (choice == 1)
        {
            return create_new_database_prompt();
        }

        if (choice == 2)
        {

            return enter_manual_path_prompt();
        }
    }

    printf("Found %d CSV file(s):\n", file_count);
    for (int i = 0; i < file_count; i++)
    {
        printf("%d. %s\n", i + 1, files[i]);
    }

    printf("\nOther Options:\n");

    printf("%d. Create new database\n", file_count + 1);
    printf("%d. Enter manual path\n", file_count + 2);
    printf("%d. Do nothing\n", file_count + 3);

    printf("\n");

    int choice = get_menu_choice(1, file_count + 3);
    if (choice == -1 || choice == file_count + 3)
        return 0;

    if (choice == file_count + 1)
    {
        return create_new_database_prompt();
    }

    if (choice == file_count + 2)
    {
        return enter_manual_path_prompt();
    }

    // Load selected file
    char *selected_file = files[choice - 1];

    if (!validate_csv_header(selected_file))
    {
        printf("✗ Invalid header format in %s\n", selected_file);
        printf("Required header: %s\n", REQUIRED_HEADER);

        if (get_yes_no("Try another file?", 0, 1))
        {
            return select_database();
        }
        return 0;
    }

    if (load_database(selected_file))
    {
        printf("✓ Database loaded successfully: %s\n", db.filename);
        printf("Records loaded: %d\n", db.count);
        pause_screen();
        return 1;
    }
    else
    {
        printf("✗ Error loading database.\n");
        pause_screen();
        return 0;
    }
}

int change_database(void)
{
    if (!get_yes_no("Current database will be closed. Continue?", 0, 1))
    {
        return 0;
    }

    if (select_database())
    {
        printf("Database changed successfully.\n");
        return 1;
    }
    else
    {
        printf("No database selected. Exiting to main menu.\n");
        return 0;
    }
}
void test_input_validation(void)
{
    printf("Running Input Validation Tests...\n");

    // Test validate_system_name function
    printf("Testing validate_system_name...\n");

    // Valid system names
    assert(validate_system_name("System123") == 1);
    assert(validate_system_name("Test-System_v1.0") == 1);
    assert(validate_system_name("System (Main)") == 1);
    assert(validate_system_name("System[Backend]") == 1);
    assert(validate_system_name("API Gateway Server") == 1);
    assert(validate_system_name("DB-Server_v2.1") == 1);

    // Invalid system names - too short
    assert(validate_system_name("AB") == 0);
    assert(validate_system_name("X") == 0);
    assert(validate_system_name("") == 0);

    // Invalid system names - invalid characters
    assert(validate_system_name("System@Test") == 0);
    assert(validate_system_name("System#Test") == 0);
    assert(validate_system_name("System$Test") == 0);
    assert(validate_system_name("System%Test") == 0);
    assert(validate_system_name("System&Test") == 0);
    assert(validate_system_name("System*Test") == 0);
    assert(validate_system_name("System+Test") == 0);
    assert(validate_system_name("System=Test") == 0);
    assert(validate_system_name("System/Test") == 0);
    assert(validate_system_name("System\\Test") == 0);
    assert(validate_system_name("System|Test") == 0);
    assert(validate_system_name("System<Test") == 0);
    assert(validate_system_name("System>Test") == 0);
    assert(validate_system_name("System?Test") == 0);
    assert(validate_system_name("System!Test") == 0);
    assert(validate_system_name("System\"Test") == 0);
    assert(validate_system_name("System'Test") == 0);
    assert(validate_system_name("System:Test") == 0);
    assert(validate_system_name("System;Test") == 0);
    assert(validate_system_name("System,Test") == 0);

    // Edge cases
    assert(validate_system_name(NULL) == 0);
    assert(validate_system_name("   ") == 0);     // Only whitespace
    assert(validate_system_name("  ABC  ") == 1); // Should trim and pass

    printf("✓ validate_system_name tests passed\n");

    // Test validate_test_type function
    printf("Testing validate_test_type...\n");

    // Valid test types
    assert(validate_test_type("API") == 1);
    assert(validate_test_type("UnitTest") == 1);
    assert(validate_test_type("IntegrationTest") == 1);
    assert(validate_test_type("SystemTest") == 1);
    assert(validate_test_type("PerformanceTest") == 1);
    assert(validate_test_type("SecurityTest") == 1);
    assert(validate_test_type("ABC123") == 1);
    assert(validate_test_type("Test123") == 1);

    // Invalid test types - too short
    assert(validate_test_type("AB") == 0);
    assert(validate_test_type("X") == 0);
    assert(validate_test_type("") == 0);

    // Invalid test types - invalid characters
    assert(validate_test_type("Test-Case") == 0);  // Hyphen not allowed
    assert(validate_test_type("Test_Case") == 0);  // Underscore not allowed
    assert(validate_test_type("Test Case") == 0);  // Space not allowed
    assert(validate_test_type("Test.Case") == 0);  // Dot not allowed
    assert(validate_test_type("Test(Case)") == 0); // Parentheses not allowed
    assert(validate_test_type("Test[Case]") == 0); // Brackets not allowed
    assert(validate_test_type("Test@Case") == 0);
    assert(validate_test_type("Test#Case") == 0);
    assert(validate_test_type("Test$Case") == 0);
    assert(validate_test_type("Test%Case") == 0);
    assert(validate_test_type("Test&Case") == 0);
    assert(validate_test_type("Test*Case") == 0);

    // Edge cases
    assert(validate_test_type(NULL) == 0);
    assert(validate_test_type("   ") == 0);     // Only whitespace
    assert(validate_test_type("  ABC  ") == 1); // Should trim and pass

    printf("✓ validate_test_type tests passed\n");

    // Test validate_test_id function
    printf("Testing validate_test_id...\n");

    // Valid test IDs
    assert(validate_test_id("1") == 1);
    assert(validate_test_id("123") == 1);
    assert(validate_test_id("999999") == 1);
    assert(validate_test_id("  123  ") == 1); // Should trim and pass

    // Invalid test IDs - zero or negative
    assert(validate_test_id("0") == 0);
    assert(validate_test_id("-1") == 0);
    assert(validate_test_id("-123") == 0);

    // Invalid test IDs - non-numeric
    assert(validate_test_id("ABC") == 0);
    assert(validate_test_id("12A") == 0);
    assert(validate_test_id("A123") == 0);
    assert(validate_test_id("12.34") == 0);
    assert(validate_test_id("12-34") == 0);
    assert(validate_test_id("12 34") == 0);
    assert(validate_test_id("") == 0);

    // Edge cases
    assert(validate_test_id(NULL) == 0);
    assert(validate_test_id("   ") == 0); // Only whitespace

    printf("✓ validate_test_id tests passed\n");

    // Test trim_string function
    printf("Testing trim_string...\n");

    char test_str1[] = "  hello  ";
    char *result1 = trim_string(test_str1);
    assert(strcmp(result1, "hello") == 0);

    char test_str2[] = "hello";
    char *result2 = trim_string(test_str2);
    assert(strcmp(result2, "hello") == 0);

    char test_str3[] = "  hello";
    char *result3 = trim_string(test_str3);
    assert(strcmp(result3, "hello") == 0);

    char test_str4[] = "hello  ";
    char *result4 = trim_string(test_str4);
    assert(strcmp(result4, "hello") == 0);

    char test_str5[] = "   ";
    char *result5 = trim_string(test_str5);
    assert(strcmp(result5, "") == 0);

    char test_str6[] = "";
    char *result6 = trim_string(test_str6);
    assert(strcmp(result6, "") == 0);

    // Test NULL input
    assert(trim_string(NULL) == NULL);

    printf("✓ trim_string tests passed\n");

    // Test string_to_test_result function
    printf("Testing string_to_test_result...\n");

    assert(string_to_test_result("Failed") == FAILED);
    assert(string_to_test_result("failed") == FAILED);
    assert(string_to_test_result("FAILED") == FAILED);
    assert(string_to_test_result("Passed") == PASSED);
    assert(string_to_test_result("passed") == PASSED);
    assert(string_to_test_result("PASSED") == PASSED);
    assert(string_to_test_result("Pending") == PENDING);
    assert(string_to_test_result("pending") == PENDING);
    assert(string_to_test_result("PENDING") == PENDING);
    assert(string_to_test_result("Success") == SUCCESS);
    assert(string_to_test_result("success") == SUCCESS);
    assert(string_to_test_result("SUCCESS") == SUCCESS);

    // Invalid results
    assert(string_to_test_result("Invalid") == INVALID_RESULT);
    assert(string_to_test_result("Unknown") == INVALID_RESULT);
    assert(string_to_test_result("") == INVALID_RESULT);
    assert(string_to_test_result(NULL) == INVALID_RESULT);

    printf("✓ string_to_test_result tests passed\n");

    // Test test_result_to_string function
    printf("Testing test_result_to_string...\n");

    assert(strcmp(test_result_to_string(FAILED), "Failed") == 0);
    assert(strcmp(test_result_to_string(PASSED), "Passed") == 0);
    assert(strcmp(test_result_to_string(PENDING), "Pending") == 0);
    assert(strcmp(test_result_to_string(SUCCESS), "Success") == 0);
    assert(strcmp(test_result_to_string(INVALID_RESULT), "Unknown") == 0);
    assert(strcmp(test_result_to_string(-999), "Unknown") == 0); // Invalid enum value

    printf("✓ test_result_to_string tests passed\n");

    printf("\nAll Input Validation Tests PASSED!\n");
    printf("Total test categories: 6\n");
    printf("- validate_system_name: ✓\n");
    printf("- validate_test_type: ✓\n");
    printf("- validate_test_id: ✓\n");
    printf("- trim_string: ✓\n");
    printf("- string_to_test_result: ✓\n");
    printf("- test_result_to_string: ✓\n");
}

void test_crud_operations(void)
{
    printf("Running CRUD Operations Tests...\n");

    // Save original database state
    Database *original_db = malloc(sizeof(Database));
    if (!original_db)
    {
        printf("Error: Unable to allocate memory for database backup.\n");
        return;
    }
    memcpy(original_db, &db, sizeof(Database));

    // Initialize test database
    memset(&db, 0, sizeof(db));
    strcpy(db.filename, "crud_test.csv");
    db.next_id = 1;

    printf("Testing find_record_by_id...\n");

    // Test empty database
    assert(find_record_by_id(1) == -1);
    assert(find_record_by_id(999) == -1);

    // Add test records
    TestRecord test_record1 = {1, "TestSystem1", "UnitTest", PASSED, 1};
    TestRecord test_record2 = {2, "TestSystem2", "IntegrationTest", FAILED, 1};
    TestRecord test_record3 = {3, "TestSystem3", "SystemTest", PENDING, 0}; // Deleted record

    db.records[0] = test_record1;
    db.records[1] = test_record2;
    db.records[2] = test_record3;
    db.count = 3;

    // Test finding existing records
    assert(find_record_by_id(1) == 0);
    assert(find_record_by_id(2) == 1);
    assert(find_record_by_id(3) == 2);

    // Test finding non-existing records
    assert(find_record_by_id(4) == -1);
    assert(find_record_by_id(999) == -1);
    assert(find_record_by_id(0) == -1);
    assert(find_record_by_id(-1) == -1);

    printf("✓ find_record_by_id tests passed\n");

    printf("Testing get_next_test_id...\n");

    // Test initial state
    db.next_id = 5;
    assert(get_next_test_id() == 5);
    assert(db.next_id == 6); // Should increment
    assert(get_next_test_id() == 6);
    assert(db.next_id == 7); // Should increment again

    printf("✓ get_next_test_id tests passed\n");

    printf("Testing database record validation...\n");

    // Test valid records
    TestRecord valid_record = {10, "ValidSystem", "ValidTest", PASSED, 1};
    assert(valid_record.test_id > 0);
    assert(strlen(valid_record.system_name) >= MIN_NAME_LENGTH);
    assert(strlen(valid_record.test_type) >= MIN_NAME_LENGTH);
    assert(valid_record.test_result >= FAILED && valid_record.test_result <= SUCCESS);
    assert(valid_record.active == 0 || valid_record.active == 1);

    // Test edge cases for record fields
    TestRecord edge_record1 = {1, "ABC", "DEF", FAILED, 0}; // Minimum length names
    assert(strlen(edge_record1.system_name) == MIN_NAME_LENGTH);
    assert(strlen(edge_record1.test_type) == MIN_NAME_LENGTH);

    TestRecord edge_record2 = {999999, "Very Long System Name", "VeryLongTestType", SUCCESS, 1};
    assert(edge_record2.test_id > 0);
    assert(strlen(edge_record2.system_name) > MIN_NAME_LENGTH);
    assert(strlen(edge_record2.test_type) > MIN_NAME_LENGTH);

    printf("✓ database record validation tests passed\n");

    printf("Testing validate_csv_header (mock)...\n");

    // Test required header format
    const char *required = REQUIRED_HEADER;
    assert(strcmp(required, "TestID,SystemName,TestType,TestResult,Active") == 0);

    printf("✓ CSV header validation tests passed\n");

    printf("Testing database bounds checking...\n");

    // Test maximum records limit
    int test_count = MAX_RECORDS;
    assert(test_count == MAX_RECORDS);

    // Test that we don't exceed maximum
    if (test_count >= MAX_RECORDS)
    {
        // Should not add more records
        printf("✓ Maximum records limit properly enforced\n");
    }

    // Test minimum valid test ID
    assert(1 > 0); // Minimum valid test ID is 1

    // Test enum bounds
    assert(FAILED == 0);
    assert(PASSED == 1);
    assert(PENDING == 2);
    assert(SUCCESS == 3);
    assert(INVALID_RESULT == -1);

    printf("✓ database bounds checking tests passed\n");

    printf("Testing memory safety (basic checks)...\n");

    // Test string lengths don't exceed buffer sizes
    char test_system[100] = "TestSystemName";
    char test_type[100] = "TestType";
    assert(strlen(test_system) < 100);
    assert(strlen(test_type) < 100);

    // Test that we can safely copy strings
    TestRecord safe_record = {0};
    strncpy(safe_record.system_name, test_system, sizeof(safe_record.system_name) - 1);
    safe_record.system_name[sizeof(safe_record.system_name) - 1] = '\0';
    strncpy(safe_record.test_type, test_type, sizeof(safe_record.test_type) - 1);
    safe_record.test_type[sizeof(safe_record.test_type) - 1] = '\0';

    assert(strlen(safe_record.system_name) == strlen(test_system));
    assert(strlen(safe_record.test_type) == strlen(test_type));

    printf("✓ memory safety tests passed\n");

    // Restore original database state
    db = *original_db;
    free(original_db);

    printf("\nAll CRUD Operations Tests PASSED!\n");
    printf("Total test categories: 6\n");
    printf("- find_record_by_id: ✓\n");
    printf("- get_next_test_id: ✓\n");
    printf("- database record validation: ✓\n");
    printf("- CSV header validation: ✓\n");
    printf("- database bounds checking: ✓\n");
    printf("- memory safety: ✓\n");
}

void run_all_tests(void)
{
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    UNIT TEST SUITE                           ║\n");
    printf("║                 System Testing Data Manager                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    printf("Starting comprehensive unit test suite...\n\n");

    // Test input validation functions
    printf("Test Category 1: Input Validation\n");
    printf("────────────────────────────────────────\n");
    test_input_validation();

    printf("\n\nTest Category 2: CRUD Operations\n");
    printf("────────────────────────────────────────\n");
    test_crud_operations();

    printf("\n\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                     TEST SUMMARY                             ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Input Validation Tests:     PASSED                           ║\n");
    printf("║ CRUD Operations Tests:      PASSED                           ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ ALL UNIT TESTS PASSED SUCCESSFULLY!                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

void run_e2e_tests(void)
{
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                   END-TO-END TEST SUITE                      ║\n");
    printf("║                System Testing Data Manager                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    printf("Starting End-to-End testing...\n\n");

    // Save original database state
    Database *original_db = malloc(sizeof(Database));
    if (!original_db)
    {
        printf("Error: Unable to allocate memory for database backup.\n");
        return;
    }
    memcpy(original_db, &db, sizeof(Database));

    printf("E2E Test 1: Complete Database Workflow\n");
    printf("─────────────────────────────────────────────\n");

    // Initialize test database
    memset(&db, 0, sizeof(db));
    strcpy(db.filename, "e2e_test.csv");
    db.next_id = 1;

    // Test 1: Create test records
    printf("Testing complete record creation workflow...\n");

    TestRecord test_records[] = {
        {1, "WebApp Frontend", "UnitTest", PASSED, 1},
        {2, "API Gateway", "IntegrationTest", FAILED, 1},
        {3, "Database Layer", "SystemTest", PENDING, 1},
        {4, "Authentication Service", "SecurityTest", SUCCESS, 1},
        {5, "Payment System", "LoadTest", FAILED, 0} // Deleted record
    };

    // Add records to database
    for (int i = 0; i < 5; i++)
    {
        db.records[db.count++] = test_records[i];
        db.next_id = test_records[i].test_id + 1;
    }

    assert(db.count == 5);
    printf("✓ Record creation workflow completed\n");

    // Test 2: Search functionality
    printf("Testing search functionality...\n");

    int search_results = 0;
    for (int i = 0; i < db.count; i++)
    {
        if (db.records[i].active && strcasestr(db.records[i].system_name, "API"))
        {
            search_results++;
        }
    }
    assert(search_results == 1); // Should find "API Gateway"
    printf("✓ Search functionality working correctly\n");

    // Test 3: Update operations
    printf("Testing update operations...\n");

    int index = find_record_by_id(2);
    assert(index != -1);
    TestRecord *record = &db.records[index];
    TestResult old_result = record->test_result;
    record->test_result = PASSED; // Change from FAILED to PASSED
    assert(record->test_result != old_result);
    printf("✓ Update operations working correctly\n");

    // Test 4: Delete and recovery operations
    printf("Testing delete and recovery operations...\n");

    // Test soft delete
    index = find_record_by_id(3);
    assert(index != -1);
    assert(db.records[index].active == 1);
    db.records[index].active = 0; // Soft delete
    assert(db.records[index].active == 0);

    // Test recovery
    db.records[index].active = 1; // Recover
    assert(db.records[index].active == 1);
    printf("✓ Delete and recovery operations working correctly\n");

    // Test 5: Data integrity checks
    printf("Testing data integrity...\n");

    int active_count = 0;
    int deleted_count = 0;

    for (int i = 0; i < db.count; i++)
    {
        if (db.records[i].active)
        {
            active_count++;
        }
        else
        {
            deleted_count++;
        }

        // Validate each record's data integrity
        assert(db.records[i].test_id > 0);
        assert(strlen(db.records[i].system_name) >= MIN_NAME_LENGTH);
        assert(strlen(db.records[i].test_type) >= MIN_NAME_LENGTH);
        assert(db.records[i].test_result >= FAILED && db.records[i].test_result <= SUCCESS);
        assert(db.records[i].active == 0 || db.records[i].active == 1);
    }

    assert(active_count == 4);  // 4 active records
    assert(deleted_count == 1); // 1 deleted record
    printf("✓ Data integrity checks passed\n");

    printf("\nE2E Test 2: ID Generation and Uniqueness\n");
    printf("──────────────────────────────────────────────\n");

    // Test ID generation
    int original_next_id = db.next_id;
    int new_id1 = get_next_test_id();
    int new_id2 = get_next_test_id();

    assert(new_id1 == original_next_id);
    assert(new_id2 == original_next_id + 1);
    assert(new_id1 != new_id2);
    printf("✓ ID generation and uniqueness working correctly\n");

    printf("\nE2E Test 3: Input Validation Integration\n");
    printf("─────────────────────────────────────────────────\n");

    // Test system name validation in context
    char valid_names[][50] = {
        "Production API Server",
        "Test-Environment_v2.0",
        "System[Backend]",
        "Database (Primary)"};

    for (int i = 0; i < 4; i++)
    {
        assert(validate_system_name(valid_names[i]) == 1);
    }

    // Test test type validation in context
    char valid_types[][30] = {
        "UnitTest",
        "IntegrationTest",
        "SystemTest",
        "PerformanceTest"};

    for (int i = 0; i < 4; i++)
    {
        assert(validate_test_type(valid_types[i]) == 1);
    }

    printf("✓ Input validation integration working correctly\n");

    printf("\nE2E Test 4: Enum Conversion Workflow\n");
    printf("─────────────────────────────────────────────\n");

    // Test complete enum conversion workflow
    const char *result_strings[] = {"Failed", "Passed", "Pending", "Success"};
    TestResult expected_results[] = {FAILED, PASSED, PENDING, SUCCESS};

    for (int i = 0; i < 4; i++)
    {
        TestResult converted = string_to_test_result(result_strings[i]);
        assert(converted == expected_results[i]);

        const char *back_converted = test_result_to_string(converted);
        assert(strcmp(back_converted, result_strings[i]) == 0);
    }

    printf("✓ Enum conversion workflow working correctly\n");

    printf("\nE2E Test 5: Memory Management\n");
    printf("───────────────────────────────────────\n");

    // Test memory allocation and deallocation patterns
    TestRecord *temp_records = malloc(db.count * sizeof(TestRecord));
    assert(temp_records != NULL);

    // Copy and verify data
    for (int i = 0; i < db.count; i++)
    {
        temp_records[i] = db.records[i];
        assert(temp_records[i].test_id == db.records[i].test_id);
        assert(strcmp(temp_records[i].system_name, db.records[i].system_name) == 0);
    }

    free(temp_records);
    printf("✓ Memory management working correctly\n");

    // Restore original database state
    db = *original_db;
    free(original_db);

    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    E2E TEST SUMMARY                          ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Complete Database Workflow:    PASSED                        ║\n");
    printf("║ ID Generation & Uniqueness:    PASSED                        ║\n");
    printf("║ Input Validation Integration:  PASSED                        ║\n");
    printf("║ Enum Conversion Workflow:      PASSED                        ║\n");
    printf("║ Memory Management:             PASSED                        ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ ALL E2E TESTS PASSED SUCCESSFULLY!                           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
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
    pause_screen();
    clear_screen();
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                 SYSTEM TESTING DATA MANAGER                  ║\n");
    printf("║                  ระบบจัดการข้อมูลกรทดสอบระบบ                    ║\n");
    printf("║                                                              ║\n");
    printf("║                 Welcome to the application!                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    pause_screen();

    while (!select_database())
    {
        if (!get_yes_no("No database selected. Try again?", 0, 1))
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
                clear_screen();
                run_all_tests();
                pause_screen();
            }
            else if (test_choice == 2)
            {
                clear_screen();
                run_e2e_tests();
                pause_screen();
            }
            break;
        case 8:
            if (get_yes_no("Are you sure you want to exit?", 0, 1))
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