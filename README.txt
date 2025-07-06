The assignment implements a library management system in C++ using object oriented programming paradigms. The code is present in main.cpp. There are 3 kinds of users and each of them have different functionalities and specifications. They are outlined in the Problem Statement. All functionalities of the statement have been implemented acccordingly. There are 2 csv files which serve as databases for the users and books and a new csv file is generated for every user that borrow's 1 or more books.

users.csv stores [userId,name,role]. The system automatically grantes required access based on the userId.
books.csv stores the details of all books in the library. This file stores the attributes [id,title,author,isbn,publisher,year, status]. One row in the file stores the data of one book.
user_userId.csv (e.g. user_1001.csv) stores [bookId,borrowDate,dueDate, returned]. The borrowDate and duedDate (calculated in minutes) will help us later in calculating the fine for users. Returned is 0 if the book os not retruned and 1 if it is returned.
To run the system, type the following commands in the console:

g++ main.cpp -o main
.\main
The system will run on the console. The data modified throughout the program run will be reflected in the csv files.
