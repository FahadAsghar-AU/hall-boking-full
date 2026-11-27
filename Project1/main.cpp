#include <iostream>
#include <string>
using namespace std;

// ==================== GLOBAL CONSTANTS ====================
const int MAX_HALLS = 10;
const int MAX_MENUS = 3;
const int MAX_SLOTS = 3;
const int MAX_MENU_ITEMS = 6;
const int MAX_BOOKINGS = 100;

const int MIN_YEAR = 2026;
const int MAX_YEAR = 2035;
const double WEEKEND_SURCHARGE = 25000.0;
const double BULK_DISCOUNT_RATE = 0.10;
const int SERVER_PORT = 8080;
const string BOOKINGS_FILE = "bookings.txt";

// ==================== DATA STRUCTURES ====================
struct Hall {
    int id;
    string name;
    string area;
    string hours;
    int capacity;
    double basePrice;
};

struct MenuPackage {
    int id;
    string shortName;
    string name;
    double pricePerGuest;
    string items[MAX_MENU_ITEMS];
};

struct Booking {
    int id;
    string clientName;
    string clientPhone;
    int hallId;
    string hallName;
    int day;
    int month;
    int year;
    int slot;
    string theme;
    string occasion;
    string perks;
    string customRequest;
    string customRequestStatus;
    int guests;
    int menuId;
    double hallBasePrice;
    double cateringCost;
    double weekendSurcharge;
    double discountAmount;
    double totalCost;
};

// ==================== EXTERNAL FUNCTION PROTOTYPES ====================
void initHalls(Hall halls[]);
void initMenus(MenuPackage menus[]);
void loadBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[], string filename);
bool saveBookings(const Booking bookings[], int totalBookings, string filename);
int readIntSafe(string prompt, int minVal, int maxVal);
void startHttpServer(Hall halls[], MenuPackage menus[], Booking bookings[], int& totalBookings);
void runConsoleMenu(Hall halls[], MenuPackage menus[], Booking bookings[], int& totalBookings);

// ==================== MAIN ENTRY POINT ====================
int main() {
    cout << fixed;

    Hall halls[MAX_HALLS];
    MenuPackage menus[MAX_MENUS];
    Booking bookings[MAX_BOOKINGS];
    int totalBookings = 0;

    initHalls(halls);
    initMenus(menus);
    loadBookings(bookings, totalBookings, halls, menus, BOOKINGS_FILE);

    int modeChoice = 0;
    do {
        cout << "\n======================================================" << endl;
        cout << "          HALL BOOKING SYSTEM - BACKEND               " << endl;
        cout << "======================================================" << endl;
        cout << "  Total Venues    : " << MAX_HALLS << " Halls (7 Islamabad Sectors)" << endl;
        cout << "  Loaded Bookings : " << totalBookings << " (File: " << BOOKINGS_FILE << ")" << endl;
        cout << "------------------------------------------------------" << endl;
        cout << "  [1] Start HTTP Server for Web UI (Port " << SERVER_PORT << ")" << endl;
        cout << "  [2] Run Console Menu Mode (Terminal App)" << endl;
        cout << "  [0] Exit" << endl;
        cout << "------------------------------------------------------" << endl;

        modeChoice = readIntSafe("Enter your choice (0-2): ", 0, 2);

        switch (modeChoice) {
        case 1:
            startHttpServer(halls, menus, bookings, totalBookings);
            break;
        case 2:
            runConsoleMenu(halls, menus, bookings, totalBookings);
            break;
        case 0:
            cout << "Saving data and exiting Hall Booking System. Goodbye!" << endl;
            saveBookings(bookings, totalBookings, BOOKINGS_FILE);
            break;
        }
    } while (modeChoice != 0);

    return 0;
}
