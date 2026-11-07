#include <iostream>
#include <string>
#include <limits>
#include "sqlite3.h"
using namespace std;

sqlite3* db = nullptr;

void execSQL(const char* sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cout << "SQL error: " << (errMsg ? errMsg : "unknown") << "\n";
        sqlite3_free(errMsg);
    }
}

void createTables() {
    execSQL("PRAGMA foreign_keys = ON;");
    execSQL(
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY,"
        "title TEXT NOT NULL,"
        "author TEXT NOT NULL,"
        "available INTEGER NOT NULL DEFAULT 1);");
    execSQL(
        "CREATE TABLE IF NOT EXISTS members ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL);");
    execSQL(
        "CREATE TABLE IF NOT EXISTS loans ("
        "loan_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "book_id INTEGER NOT NULL,"
        "member_id INTEGER NOT NULL,"
        "issue_date TEXT DEFAULT CURRENT_DATE,"
        "return_date TEXT,"
        "FOREIGN KEY(book_id) REFERENCES books(id),"
        "FOREIGN KEY(member_id) REFERENCES members(id));");
}

bool exists(const char* sql, int id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

void addBook() {
    int id; string title, author;
    cout << "Book ID: "; cin >> id;
    if (exists("SELECT 1 FROM books WHERE id = ?;", id)) {
        cout << "Book ID already exists.\n"; return;
    }
    cin.ignore();
    cout << "Title: "; getline(cin, title);
    cout << "Author: "; getline(cin, author);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO books (id, title, author) VALUES (?, ?, ?);", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, author.c_str(), -1, SQLITE_TRANSIENT);
    cout << (sqlite3_step(stmt) == SQLITE_DONE ? "Book added.\n" : "Failed to add book.\n");
    sqlite3_finalize(stmt);
}

void listBooks() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, title, author, available FROM books ORDER BY id;", -1, &stmt, nullptr);
    cout << "\nID\tTitle\t\tAuthor\t\tStatus\n";
    cout << "--------------------------------------------------\n";
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        cout << sqlite3_column_int(stmt, 0) << "\t"
             << sqlite3_column_text(stmt, 1) << "\t\t"
             << sqlite3_column_text(stmt, 2) << "\t\t"
             << (sqlite3_column_int(stmt, 3) ? "Available" : "Issued") << "\n";
    }
    if (!found) cout << "No books found.\n";
    sqlite3_finalize(stmt);
}

void searchBook() {
    string keyword;
    cin.ignore();
    cout << "Search title or author: "; getline(cin, keyword);
    keyword = "%" + keyword + "%";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, title, author, available FROM books "
        "WHERE title LIKE ? OR author LIKE ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, keyword.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, keyword.c_str(), -1, SQLITE_TRANSIENT);
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        cout << sqlite3_column_int(stmt, 0) << " | "
             << sqlite3_column_text(stmt, 1) << " | "
             << sqlite3_column_text(stmt, 2) << " | "
             << (sqlite3_column_int(stmt, 3) ? "Available" : "Issued") << "\n";
    }
    if (!found) cout << "No matching books.\n";
    sqlite3_finalize(stmt);
}

void deleteBook() {
    int id;
    cout << "Book ID to delete: "; cin >> id;
    if (!exists("SELECT 1 FROM books WHERE id = ?;", id)) {
        cout << "Book not found.\n"; return;
    }
    if (exists("SELECT 1 FROM loans WHERE book_id = ?;", id)) {
        cout << "Cannot delete: this book has loan records.\n"; return;
    }
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "DELETE FROM books WHERE id = ?;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "Book deleted.\n";
}

void addMember() {
    int id; string name;
    cout << "Member ID: "; cin >> id;
    if (exists("SELECT 1 FROM members WHERE id = ?;", id)) {
        cout << "Member ID already exists.\n"; return;
    }
    cin.ignore();
    cout << "Name: "; getline(cin, name);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO members (id, name) VALUES (?, ?);", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    cout << (sqlite3_step(stmt) == SQLITE_DONE ? "Member added.\n" : "Failed to add member.\n");
    sqlite3_finalize(stmt);
}

void listMembers() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id, name FROM members ORDER BY id;", -1, &stmt, nullptr);
    cout << "\nID\tName\n--------------------\n";
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        cout << sqlite3_column_int(stmt, 0) << "\t"
             << sqlite3_column_text(stmt, 1) << "\n";
    }
    if (!found) cout << "No members found.\n";
    sqlite3_finalize(stmt);
}

void issueBook() {
    int bookId, memberId;
    cout << "Book ID: "; cin >> bookId;
    cout << "Member ID: "; cin >> memberId;
    if (!exists("SELECT 1 FROM books WHERE id = ?;", bookId)) {
        cout << "Book not found.\n"; return;
    }
    if (!exists("SELECT 1 FROM members WHERE id = ?;", memberId)) {
        cout << "Member not found.\n"; return;
    }
    if (!exists("SELECT 1 FROM books WHERE id = ? AND available = 1;", bookId)) {
        cout << "Book is already issued.\n"; return;
    }
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO loans (book_id, member_id) VALUES (?, ?);", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, bookId);
    sqlite3_bind_int(stmt, 2, memberId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "UPDATE books SET available = 0 WHERE id = ?;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, bookId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "Book issued successfully.\n";
}

void returnBook() {
    int bookId;
    cout << "Book ID to return: "; cin >> bookId;
    if (!exists("SELECT 1 FROM loans WHERE book_id = ? AND return_date IS NULL;", bookId)) {
        cout << "This book is not currently issued.\n"; return;
    }
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "UPDATE loans SET return_date = CURRENT_DATE "
        "WHERE book_id = ? AND return_date IS NULL;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, bookId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "UPDATE books SET available = 1 WHERE id = ?;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, bookId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "Book returned successfully.\n";
}

void showIssued() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT b.id, b.title, m.name, l.issue_date "
        "FROM loans l JOIN books b ON l.book_id = b.id "
        "JOIN members m ON l.member_id = m.id "
        "WHERE l.return_date IS NULL ORDER BY l.issue_date;", -1, &stmt, nullptr);
    cout << "\nBook ID\tTitle\t\tIssued To\tDate\n";
    cout << "--------------------------------------------------\n";
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        cout << sqlite3_column_int(stmt, 0) << "\t"
             << sqlite3_column_text(stmt, 1) << "\t\t"
             << sqlite3_column_text(stmt, 2) << "\t\t"
             << sqlite3_column_text(stmt, 3) << "\n";
    }
    if (!found) cout << "No books are currently issued.\n";
    sqlite3_finalize(stmt);
}

int main() {
    if (sqlite3_open("library.db", &db) != SQLITE_OK) {
        cout << "Cannot open database.\n";
        return 1;
    }
    createTables();

    int choice;
    do {
        cout << "\n=== Library Management System ===\n";
        cout << "1. Add Book\n";
        cout << "2. List Books\n";
        cout << "3. Search Book\n";
        cout << "4. Delete Book\n";
        cout << "5. Add Member\n";
        cout << "6. List Members\n";
        cout << "7. Issue Book\n";
        cout << "8. Return Book\n";
        cout << "9. Show Issued Books\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            choice = -1;
        }

        switch (choice) {
            case 1: addBook(); break;
            case 2: listBooks(); break;
            case 3: searchBook(); break;
            case 4: deleteBook(); break;
            case 5: addMember(); break;
            case 6: listMembers(); break;
            case 7: issueBook(); break;
            case 8: returnBook(); break;
            case 9: showIssued(); break;
            case 0: cout << "Goodbye!\n"; break;
            default: cout << "Invalid choice. Try again.\n";
        }
    } while (choice != 0);

    sqlite3_close(db);
    return 0;
}