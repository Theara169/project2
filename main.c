#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sqlite3.h"

struct Employee {
    char employee_Name[100];
    int employee_Age;
    char employee_Position[100];
    float employee_Salary;
    char employee_Email[50];
    char employee_Password[50];   // Password for website login
};

// Check if email ends with "@gmail.com"
bool endsWithEmailCom(struct Employee employee) {
    char *at = strchr(employee.employee_Email, '@');
    if (at == NULL) return false;
    return (strcmp(at, "@gmail.com") == 0);
}

// Print all employees (without password)
void printAllEmployees(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, name, age, position, salary, email FROM employee;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("Error preparing select: %s\n", sqlite3_errmsg(db));
        return;
    }

    printf("\n========== Employee Records ==========\n");
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        int age = sqlite3_column_int(stmt, 2);
        const unsigned char *pos = sqlite3_column_text(stmt, 3);
        double salary = sqlite3_column_double(stmt, 4);
        const unsigned char *email = sqlite3_column_text(stmt, 5);
        printf("ID: %d | Name: %s | Age: %d | Position: %s | Salary: %.2f | Email: %s\n",
               id, name, age, pos, salary, email);
        count++;
    }
    if (count == 0) printf("No employees found.\n");
    printf("======================================\n");
    sqlite3_finalize(stmt);
}

int main() {
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    // Use employee1.db (same as Flask backend)
    rc = sqlite3_open("employee1.db", &db);
    if (rc) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    printf("Database connected successfully.\n");

    // Create table with password column
    const char *createTableSQL =
        "CREATE TABLE IF NOT EXISTS employee ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "age INTEGER NOT NULL,"
        "position TEXT NOT NULL,"
        "salary REAL NOT NULL,"
        "email TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL);";
    rc = sqlite3_exec(db, createTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    int userchoice;
    char nextchoice;

    while (1) {
        printf("\n1. Add new employee\n");
        printf("2. Delete employee\n");
        printf("3. Print all employee details\n");
        printf("4. Exit the system\n");
        printf("Enter your choice: ");
        scanf("%d", &userchoice);

        switch (userchoice) {
            case 1: { // Add employee
                struct Employee emp;
                nextchoice = 'y';
                while (nextchoice == 'y' || nextchoice == 'Y') {
                    printf("\n===============================================\n");
                    
                    printf("Employee Name: ");
                    scanf(" %99[^\n]", emp.employee_Name);

                    printf("Employee Age: ");
                    scanf("%d", &emp.employee_Age);
                    while (emp.employee_Age < 18 || emp.employee_Age > 100) {
                        printf("\033[31mERROR: Invalid Age! (must be 18-100)\033[0m\n");
                        printf("Employee Age: ");
                        scanf("%d", &emp.employee_Age);
                    }

                    printf("Employee Position: ");
                    scanf(" %99[^\n]", emp.employee_Position);

                    printf("Employee Salary: ");
                    scanf("%f", &emp.employee_Salary);

                    bool validEmail = false;
                    while (!validEmail) {
                        printf("Employee Email: ");
                        scanf(" %49[^\n]", emp.employee_Email);
                        if (endsWithEmailCom(emp)) {
                            printf("\033[32mEmail VALID\033[0m\n");
                            validEmail = true;
                        } else {
                            printf("\033[31mERROR: Email must end with @gmail.com\033[0m\n");
                        }
                    }

                    // Prompt for password
                    printf("Employee Password (for website login): ");
                    scanf(" %49[^\n]", emp.employee_Password);

                    // Insert into database (including password)
                    sqlite3_stmt *stmt;
                    const char *insertSQL = "INSERT INTO employee (name, age, position, salary, email, password) VALUES (?, ?, ?, ?, ?, ?);";
                    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, 0);
                    if (rc != SQLITE_OK) {
                        printf("Error preparing insert: %s\n", sqlite3_errmsg(db));
                    } else {
                        sqlite3_bind_text(stmt, 1, emp.employee_Name, -1, SQLITE_STATIC);
                        sqlite3_bind_int(stmt, 2, emp.employee_Age);
                        sqlite3_bind_text(stmt, 3, emp.employee_Position, -1, SQLITE_STATIC);
                        sqlite3_bind_double(stmt, 4, emp.employee_Salary);
                        sqlite3_bind_text(stmt, 5, emp.employee_Email, -1, SQLITE_STATIC);
                        sqlite3_bind_text(stmt, 6, emp.employee_Password, -1, SQLITE_STATIC);

                        if (sqlite3_step(stmt) == SQLITE_DONE) {
                            printf("\033[32mEmployee added successfully!\033[0m\n");
                        } else {
                            printf("\033[31mError adding employee: %s\033[0m\n", sqlite3_errmsg(db));
                        }
                        sqlite3_finalize(stmt);
                    }

                    printf("Add another record? (y/n): ");
                    scanf(" %c", &nextchoice);
                }
                break;
            }

            case 2: { // Delete employee
                char deleteName[100];
                printf("Enter employee name to delete: ");
                scanf(" %99[^\n]", deleteName);

                sqlite3_stmt *stmt;
                const char *deleteSQL = "DELETE FROM employee WHERE name = ?;";
                rc = sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, 0);
                if (rc != SQLITE_OK) {
                    printf("Error preparing delete: %s\n", sqlite3_errmsg(db));
                    break;
                }
                sqlite3_bind_text(stmt, 1, deleteName, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    int changes = sqlite3_changes(db);
                    if (changes > 0)
                        printf("\033[32mEmployee '%s' deleted successfully.\033[0m\n", deleteName);
                    else
                        printf("\033[31mEmployee '%s' not found.\033[0m\n", deleteName);
                } else {
                    printf("Delete error: %s\n", sqlite3_errmsg(db));
                }
                sqlite3_finalize(stmt);
                break;
            }

            case 3:
                printAllEmployees(db);
                break;

            case 4:
                printf("Exiting system...\n");
                sqlite3_close(db);
                return 0;

            default:
                printf("\033[31mERROR: Invalid choice!\033[0m\n");
        }
    }
    return 0;
}