#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Function declarations

// Display functions
void display_welcome_message(void);
void clear_screen(void);
void pause_screen(void);

// Main menu functions
void show_main_menu(void);

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