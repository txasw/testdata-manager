#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Constants
#define MAX_FILES 100
#define MAX_PATH 260
#define MAX_LINE 1024
#define MAX_RECORDS 10000
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

int get_menu_choice(int min, int max)
{
    char input[10];
    int choice;
    printf("Enter your choice (%d-%d): ", min, max);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';

    if (sscanf(input, "%d", &choice) == 1 && choice >= min && choice <= max)
    {
        return choice;
    }
    printf("Invalid choice. Please enter a number between %d and %d.\n", min, max);

    return -1;
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
// Main function
int main(void)
{
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
            printf("\nList all records selected.\n");
            pause_screen();
            break;
        case 2:
            printf("\nAdd new record selected.\n");
            pause_screen();
            break;
        case 3:
            printf("\nSearch records selected.\n");
            pause_screen();
            break;
        case 4:
            printf("\nUpdate record selected.\n");
            pause_screen();
            break;
        case 5:
            printf("\nRecovery data selected.\n");
            pause_screen();
            break;
        case 6:
            printf("\nChange database selected.\n");
            pause_screen();
            break;
        case 7:
            printf("\nSelect test type\n");
            pause_screen();
            break;
        case 8:
            printf("Are you sure you want to exit? (y/n): ");
            char confirm[10];
            fgets(confirm, sizeof(confirm), stdin);
            if (confirm[0] == 'y' || confirm[0] == 'Y')
            {
                printf("Thank you for using System Testing Data Manager!\n");
                return 0;
            }
            break;
        }
    }

    return 0;
}