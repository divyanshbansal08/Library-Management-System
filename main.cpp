#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <iomanip>

using namespace std;

class UserManager;
class Library;
class User;

time_t getCurrentTime() { return time(0); }
int daysBetween(time_t t1, time_t t2) { return abs(difftime(t1, t2) / 86400); }

class Book {
private:
    string isbn, publisher;
    int year;
public:
    int id;
    string title, author, status;

    Book(int id, string t, string a, string i, string p, int y, string s = "Available")
        : id(id), title(t), author(a), isbn(i), publisher(p), year(y), status(s) {}

    void display() const;
    string getCSV() const;
};

class Library {
private:
    vector<Book> books;
    const string dataFile = "books.csv";

public:
    Library();
    void loadData();
    void saveData();
    Book* findBook(int id);
    void addBook();
    void removeBook(int bookId);
    void displayAll() const;
};

class Account {
private:
    struct BorrowRecord {
        int bookId;
        time_t borrowDate;
        time_t dueDate;
        bool returned = false;
    };

    int userId;
    vector<BorrowRecord> records;
    int fineBalance = 0;
    const string dataFile;

    void saveRecords();

public:
    Account(int uid);
    const vector<BorrowRecord>& getBorrowRecords() const;
    int getFineBalance() const;
    void borrowBook(int bookId, int borrowDays);
    int calculateFines(bool isStudent);
    bool returnBook(int bookId, Library& lib);
    vector<int> getCurrentBooks() const;
    int getCurrentBorrowedCount() const;
    void payFine(int amount);
    void display() const;
};

class User {
protected:
    int id;
    string name;
    Account account;
    string role;

public:
    User(int uid, string n, string r);
    virtual ~User() = default;

    virtual bool canBorrow(Library& lib) = 0;
    virtual void showMenu(Library& lib, UserManager& umgr) = 0;

    void viewAccount() const;
    string getRole() const;
    string getName() const;
    int getId() const;
};

class Student : public User {
public:
    Student(int id, string name);
    bool canBorrow(Library& lib) override;
    void showMenu(Library& lib, UserManager& umgr) override;
};

class Faculty : public User {
public:
    Faculty(int id, string name);
    bool canBorrow(Library& lib) override;
    void showMenu(Library& lib, UserManager& umgr) override;
};

class Librarian : public User {
public:
    Librarian(int id, string name);
    bool canBorrow(Library& lib) override;
    void showMenu(Library& lib, UserManager& umgr) override;
};

class UserManager {
private:
    map<int, unique_ptr<User>> users;
    const string dataFile = "users.csv";

    void loadUsers();
    void saveAllUsers();

public:
    UserManager();
    void initializeDefaultUsers();
    void addUser();
    void removeUser(int userId);
    User* authenticate();
};

void Book::display() const {
    cout << "ID: " << id << " | Title: " << title << " | Author: " << author
         << " | ISBN: " << isbn << " | Status: " << status << endl;
}

string Book::getCSV() const {
    return to_string(id) + "," + title + "," + author + "," + isbn + ","
           + publisher + "," + to_string(year) + "," + status;
}

Library::Library() { loadData(); }

void Library::loadData() {
    ifstream file(dataFile);
    if (!file) {
        cerr << "Error: books.csv not found!\n";
        return;
    }

    string line;
    bool firstLine = true;
    while (getline(file, line)) {
        if (firstLine) { firstLine = false; continue; }

        stringstream ss(line);
        vector<string> fields;
        string field;

        while(getline(ss, field, ',')) fields.push_back(field);

        if(fields.size() == 7) {
            books.emplace_back(stoi(fields[0]), fields[1], fields[2], fields[3],
                             fields[4], stoi(fields[5]), fields[6]);
        }
    }
    file.close();
}

void Library::saveData() {
    ofstream file(dataFile);
    file << "id,title,author,isbn,publisher,year,status\n";
    for (auto& b : books) file << b.getCSV() << "\n";
    file.close();
}

Book* Library::findBook(int id) {
    auto it = find_if(books.begin(), books.end(),
        [id](const Book& b) { return b.id == id; });
    return it != books.end() ? &(*it) : nullptr;
}

void Library::addBook() {
    int id, year;
    string title, author, isbn, publisher;

    cout << "Enter Book ID: "; cin >> id;
    cin.ignore();
    cout << "Title: "; getline(cin, title);
    cout << "Author: "; getline(cin, author);
    cout << "ISBN: "; getline(cin, isbn);
    cout << "Publisher: "; getline(cin, publisher);
    cout << "Year: "; cin >> year;

    books.emplace_back(id, title, author, isbn, publisher, year);
    saveData();
    cout << "Book added successfully!\n";
}

void Library::removeBook(int bookId) {
    auto it = remove_if(books.begin(), books.end(),
        [bookId](const Book& b) { return b.id == bookId; });

    if(it != books.end()) {
        books.erase(it, books.end());
        saveData();
        cout << "Book removed successfully!\n";
    } else {
        cout << "Book not found!\n";
    }
}

void Library::displayAll() const {
    cout << "\n=== Library Catalog ===\n";
    for (const auto& b : books) b.display();
}

Account::Account(int uid) : userId(uid), dataFile("user_" + to_string(uid) + ".csv") {
    ifstream file(dataFile);
    if (file) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            vector<string> fields;
            string field;

            while(getline(ss, field, ',')) fields.push_back(field);

            if(fields.size() == 4) {
                records.push_back({
                    stoi(fields[0]),
                    stol(fields[1]),
                    stol(fields[2]),
                    static_cast<bool>(stoi(fields[3]))
                });
            }
        }
    }
}

void Account::saveRecords() {
    ofstream file(dataFile);
    for (auto& r : records) {
        file << r.bookId << "," << r.borrowDate << ","
             << r.dueDate << "," << r.returned << "\n";
    }
    file.close();
}

const vector<Account::BorrowRecord>& Account::getBorrowRecords() const { return records; }
int Account::getFineBalance() const { return fineBalance; }

void Account::borrowBook(int bookId, int borrowDays) {
    time_t now = getCurrentTime();
    records.push_back({bookId, now, now + borrowDays * 86400, false});
    saveRecords();
}

int Account::calculateFines(bool isStudent) {
    time_t now = getCurrentTime();
    int fines = 0;

    for (auto& r : records) {
        if (!r.returned && now > r.dueDate) {
            int overdueDays = daysBetween(now, r.dueDate);
            fines += isStudent ? overdueDays * 10 : 0;
        }
    }
    fineBalance = fines;
    return fines;
}

bool Account::returnBook(int bookId, Library& lib) {
    auto it = find_if(records.begin(), records.end(),
        [bookId](const BorrowRecord& r) { return r.bookId == bookId && !r.returned; });

    if (it != records.end()) {
        time_t now = getCurrentTime();
        if (now > it->dueDate) {
            int overdueDays = daysBetween(now, it->dueDate);
            fineBalance += overdueDays * 10;
        }

        it->returned = true;
        if (auto book = lib.findBook(bookId)) {
            book->status = "Available";
            lib.saveData();
        }
        saveRecords();
        calculateFines(true);
        return true;
    }
    return false;
}

vector<int> Account::getCurrentBooks() const {
    vector<int> current;
    for (const auto& r : records) {
        if (!r.returned) current.push_back(r.bookId);
    }
    return current;
}

int Account::getCurrentBorrowedCount() const {
    return getCurrentBooks().size();
}

void Account::payFine(int amount) {
    fineBalance = max(fineBalance - amount, 0);
    cout << "Paid Rs." << amount << ". Remaining balance: Rs." << fineBalance << endl;
}

void Account::display() const {
    cout << "\n=== Account Summary ===\n";
    cout << "User ID: " << userId << endl;
    cout << "Current Fines: Rs." << fineBalance << endl;
    cout << "Borrowed Books: ";
    for (auto id : getCurrentBooks()) cout << id << " ";
    cout << endl;
}

User::User(int uid, string n, string r) : id(uid), name(n), account(uid), role(r) {}

void User::viewAccount() const { account.display(); }
string User::getRole() const { return role; }
string User::getName() const { return name; }
int User::getId() const { return id; }

Student::Student(int id, string name) : User(id, name, "Student") {}

bool Student::canBorrow(Library& lib) {
    account.calculateFines(true);
    if (account.getFineBalance() > 0) {
        cout << "Outstanding fines: Rs." << account.getFineBalance()
             << ". Pay fines first!\n";
        return false;
    }
    if (account.getCurrentBorrowedCount() >= 3) {
        cout << "Maximum 3 books allowed!\n";
        return false;
    }
    return true;
}

void Student::showMenu(Library& lib, UserManager& umgr) {
    while(true) {
        account.calculateFines(true);

        cout << "\n=== Student Menu ===\n"
             << "1. View Available Books\n"
             << "2. Borrow Book (15 days)\n"
             << "3. Return Book\n"
             << "4. View Account\n"
             << "5. Pay Fines\n"
             << "6. Logout\n"
             << "Choice: ";

        int choice;
        cin >> choice;

        switch(choice) {
            case 1: lib.displayAll(); break;
            case 2: {
                if (!canBorrow(lib)) break;
                int bookId;
                cout << "Enter Book ID: "; cin >> bookId;
                if (auto book = lib.findBook(bookId)) {
                    if (book->status == "Available") {
                        account.borrowBook(bookId, 15);
                        book->status = "Borrowed";
                        lib.saveData();
                        cout << "Book borrowed successfully!\n";
                    } else {
                        cout << "Book not available!\n";
                    }
                } else {
                    cout << "Invalid Book ID!\n";
                }
                break;
            }
            case 3: {
                int bookId;
                cout << "Enter Book ID to return: "; cin >> bookId;
                if (account.returnBook(bookId, lib)) {
                    cout << "Book returned successfully!\n";
                } else {
                    cout << "Return failed! Check Book ID.\n";
                }
                break;
            }
            case 4: viewAccount(); break;
            case 5: {
                int amount;
                cout << "Enter amount to pay: "; cin >> amount;
                account.payFine(amount);
                break;
            }
            case 6: return;
            default: cout << "Invalid choice!\n";
        }
    }
}
Faculty::Faculty(int id, string name) : User(id, name, "Faculty") {}

bool Faculty::canBorrow(Library& lib) {
    if (account.getCurrentBorrowedCount() >= 5) {
        cout << "Maximum 5 books allowed!\n";
        return false;
    }

    time_t now = getCurrentTime();
    for (const auto& record : account.getBorrowRecords()) {
        if (!record.returned) {
            int overdueDays = daysBetween(now, record.dueDate);
            if (overdueDays > 30) {
                cout << "Overdue book >30 days. Return it first!\n";
                return false;
            }
        }
    }
    return true;
}

void Faculty::showMenu(Library& lib, UserManager& umgr) {
    while(true) {
        account.calculateFines(false);

        cout << "\n=== Faculty Menu ===\n"
             << "1. View Available Books\n"
             << "2. Borrow Book (30 days)\n"
             << "3. Return Book\n"
             << "4. View Account\n"
             << "5. Logout\n"
             << "Choice: ";

        int choice;
        cin >> choice;

        switch(choice) {
            case 1: lib.displayAll(); break;
            case 2: {
                if (!canBorrow(lib)) break;
                int bookId;
                cout << "Enter Book ID: "; cin >> bookId;
                if (auto book = lib.findBook(bookId)) {
                    if (book->status == "Available") {
                        account.borrowBook(bookId, 30);
                        book->status = "Borrowed";
                        lib.saveData();
                        cout << "Book borrowed successfully!\n";
                    } else {
                        cout << "Book not available!\n";
                    }
                } else {
                    cout << "Invalid Book ID!\n";
                }
                break;
            }
            case 3: {
                int bookId;
                cout << "Enter Book ID to return: "; cin >> bookId;
                if (account.returnBook(bookId, lib)) {
                    cout << "Book returned successfully!\n";
                } else {
                    cout << "Return failed! Check Book ID.\n";
                }
                break;
            }
            case 4: viewAccount(); break;
            case 5: return;
            default: cout << "Invalid choice!\n";
        }
    }
}
Librarian::Librarian(int id, string name) : User(id, name, "Librarian") {}

bool Librarian::canBorrow(Library&) { return false; }

void Librarian::showMenu(Library& lib, UserManager& umgr) {
    while(true) {
        cout << "\n=== Librarian Menu ===\n"
             << "1. Add Book\n"
             << "2. Remove Book\n"
             << "3. View All Books\n"
             << "4. Add User\n"
             << "5. Remove User\n"
             << "6. Logout\n"
             << "Choice: ";

        int choice;
        cin >> choice;

        switch(choice) {
            case 1: lib.addBook(); break;
            case 2: {
                int bookId;
                cout << "Enter Book ID to remove: "; cin >> bookId;
                lib.removeBook(bookId);
                break;
            }
            case 3: lib.displayAll(); break;
            case 4: umgr.addUser(); break;
            case 5: {
                int userId;
                cout << "Enter User ID to remove: "; cin >> userId;
                umgr.removeUser(userId);
                break;
            }
            case 6: return;
            default: cout << "Invalid choice!\n";
        }
    }
}

UserManager::UserManager() {
    loadUsers();
    if (users.empty()) initializeDefaultUsers();
}

void UserManager::loadUsers() {
    ifstream file(dataFile);
    if (!file) return;

    string line;
    bool firstLine = true;
    while (getline(file, line)) {
        if (firstLine) { firstLine = false; continue; }

        stringstream ss(line);
        vector<string> fields;
        string field;

        while(getline(ss, field, ',')) fields.push_back(field);

        if(fields.size() == 3) {
            int id = stoi(fields[0]);
            string name = fields[1];
            string role = fields[2];

            if (role == "Student") {
                users[id] = make_unique<Student>(id, name);
            } else if (role == "Faculty") {
                users[id] = make_unique<Faculty>(id, name);
            } else if (role == "Librarian") {
                users[id] = make_unique<Librarian>(id, name);
            }
        }
    }
    file.close();
}

void UserManager::initializeDefaultUsers() {
    ofstream file(dataFile);
    file << "userid,name,role\n"
         << "1001,John Doe,Student\n"
         << "1002,Jane Smith,Student\n"
         << "1003,Alice Johnson,Student\n"
         << "1004,Bob Wilson,Student\n"
         << "1005,Charlie Brown,Student\n"
         << "2001,Dr. Smith,Faculty\n"
         << "2002,Dr. Emily Davis,Faculty\n"
         << "2003,Dr. Michael Lee,Faculty\n"
         << "3001,Librarian Admin,Librarian\n";
    file.close();
    loadUsers();
}

void UserManager::addUser() {
    int newId;
    string name, role;

    cout << "Enter new user ID: "; cin >> newId;
    if (users.find(newId) != users.end()) {
        cout << "User ID already exists!\n";
        return;
    }

    cin.ignore();
    cout << "Enter name: "; getline(cin, name);
    cout << "Enter role (Student/Faculty/Librarian): "; cin >> role;

    transform(role.begin(), role.end(), role.begin(), ::tolower);
    if (role != "student" && role != "faculty" && role != "librarian") {
        cout << "Invalid role!\n";
        return;
    }

    role[0] = toupper(role[0]);
    ofstream file(dataFile, ios::app);
    file << newId << "," << name << "," << role << "\n";
    file.close();

    if (role == "Student") {
        users[newId] = make_unique<Student>(newId, name);
    } else if (role == "Faculty") {
        users[newId] = make_unique<Faculty>(newId, name);
    } else {
        users[newId] = make_unique<Librarian>(newId, name);
    }

    cout << "User added successfully!\n";
}

void UserManager::removeUser(int userId) {
    auto it = users.find(userId);
    if (it == users.end()) {
        cout << "User not found!\n";
        return;
    }

    users.erase(it);
    saveAllUsers();
    cout << "User removed successfully!\n";
}

void UserManager::saveAllUsers() {
    ofstream file(dataFile);
    file << "userid,name,role\n";
    for (const auto& [id, user] : users) {
        file << id << "," << user->getName() << "," << user->getRole() << "\n";
    }
    file.close();
}

User* UserManager::authenticate() {
    int userId;
    cout << "Enter User ID: "; cin >> userId;

    auto it = users.find(userId);
    if (it != users.end()) {
        return it->second.get();
    }
    cout << "User not found!\n";
    return nullptr;
}

int main() {
    Library library;
    UserManager umgr;
    ifstream bookCheck("books.csv");
    if(bookCheck.peek() == ifstream::traits_type::eof()) {
        ofstream initBooks("books.csv");
        initBooks << "id,title,author,isbn,publisher,year,status\n"
                 << "101,Clean Code,Robert Martin,978-0132350884,Prentice Hall,2008,Available\n"
                 << "102,Design Patterns,Gamma et al.,978-0201633610,Addison-Wesley,1994,Available\n"
                 << "103,The C++ Programming Language,Bjarne Stroustrup,978-0321563842,Addison-Wesley,2013,Available\n"
                 << "104,Effective Modern C++,Scott Meyers,978-1491903995,O'Reilly,2014,Available\n"
                 << "105,Refactoring,Martin Fowler,978-0134757599,Addison-Wesley,2018,Available\n"
                 << "106,Code Complete,Steve McConnell,978-0735619678,Microsoft Press,2004,Available\n"
                 << "107,The Pragmatic Programmer,Andrew Hunt,978-0201616224,Addison-Wesley,1999,Available\n"
                 << "108,Introduction to Algorithms,Thomas Cormen,978-0262033848,MIT Press,2009,Available\n"
                 << "109,Structure and Interpretation of Computer Programs,Gerald Sussman,978-0262510875,MIT Press,1996,Available\n"
                 << "110,Artificial Intelligence: A Modern Approach,Stuart Russell,978-0136042594,Prentice Hall,2010,Available\n";
    }

    while(true) {
        cout << "\n=== Library System Login ===\n";
        User* user = umgr.authenticate();

        if (!user) continue;

        user->showMenu(library, umgr);
    }

    return 0;
}
