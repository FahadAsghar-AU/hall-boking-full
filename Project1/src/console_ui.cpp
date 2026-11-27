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

// ==================== HELPER PROTOTYPES ====================
bool isLeapYear(int year);
int daysInMonth(int month, int year);
bool isValidDate(int day, int month, int year);
int getDayOfWeek(int day, int month, int year);
bool isWeekendDate(int day, int month, int year);
bool isValidPhone(string phone);
string sanitizeText(string text);
string toLowerString(string text);
string toUpperString(string text);
int parseIntSafe(string text, int defaultValue);
double parseDoubleSafe(string text, double defaultValue);
bool isKnownSector(string sector);
int extractSectorNumber(string sector);
int computeSectorMatchScore(string hallSector, string desiredSector);
bool isValidTheme(string theme);
string normalizeTheme(string theme);
string getOccasionPerks(string occasion);
int findHallIndexById(const Hall halls[], int hallId);
int findMenuIndexById(const MenuPackage menus[], int menuId);
int findBookingIndexById(const Booking bookings[], int totalBookings, int bookingId);
string getSlotName(int slot);
string getSlotTimeRange(int slot);
string formatDate(int day, int month, int year);
string formatMoney(double amount);

// ==================== BOOKING SERVICE PROTOTYPES ====================
void initHalls(Hall halls[]);
void initMenus(MenuPackage menus[]);
void seedDefaultBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[]);
bool isSlotAvailable(const Booking bookings[], int totalBookings, int hallId, int day, int month, int year, int slot, int ignoreBookingId);
void buildAvailabilityMatrix(const Booking bookings[], int totalBookings, int day, int month, int year, bool bookedSlots[][MAX_SLOTS], int bookingIdMatrix[][MAX_SLOTS], int& availableCount, int& bookedCount);
void rankHallsByFilters(const Hall halls[], const Booking bookings[], int totalBookings, string desiredSector, int guests, double maxBudget, int day, int month, int year, int desiredSlot, int rankedIndices[], int matchScores[], bool exactMatches[]);
bool qualifiesForBulkDiscount(int guests, int hallCapacity);
bool calculateBookingCost(const Hall halls[], const MenuPackage menus[], int hallId, int menuId, int guests, int day, int month, int year, double& hallBasePrice, double& cateringCost, double& weekendSurcharge, double& discountAmount, double& totalCost);
bool validateBookingInput(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings, string clientName, string clientPhone, int day, int month, int year, int slot, int hallId, int guests, int menuId, string theme, string occasion, int ignoreBookingId, string& errorMsg);
int getNextBookingId(const Booking bookings[], int totalBookings);
bool addBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings, string clientName, string clientPhone, int day, int month, int year, int slot, int hallId, int guests, int menuId, string theme, string occasion, string customRequest, Booking& createdBooking, string& errorMsg);
bool updateBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int totalBookings, int bookingId, string clientName, string clientPhone, int day, int month, int year, int slot, int hallId, int guests, int menuId, string theme, string occasion, string customRequest, string requestStatus, Booking& updatedBooking, string& errorMsg);
bool deleteBooking(Booking bookings[], int& totalBookings, int bookingId, string& errorMsg);
int searchAndFilterBookings(const Booking bookings[], int totalBookings, string searchQuery, int hallFilterId, int resultIndices[]);
void sortBookingIndices(const Booking bookings[], int indices[], int count, string sortBy, string order);

void consoleAddBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings, int preselectedHallId, int preselectedDay, int preselectedMonth, int preselectedYear, int preselectedSlot, int preselectedGuests);

// ==================== CONSOLE MENU MODE ====================
int readIntSafe(string prompt, int minVal, int maxVal) {
    int value = 0;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail() || value < minVal || value > maxVal) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "[Error] Please enter a valid number between " << minVal << " and " << maxVal << "." << endl;
        }
        else {
            cin.ignore(10000, '\n');
            return value;
        }
    }
}

double readDoubleSafe(string prompt, double minVal, double maxVal) {
    double value = 0.0;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail() || value < minVal || value > maxVal) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "[Error] Please enter a valid amount between " << (long long)minVal << " and " << (long long)maxVal << "." << endl;
        }
        else {
            cin.ignore(10000, '\n');
            return value;
        }
    }
}

string readLineSafe(string prompt) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        input = sanitizeText(input);
        if (!input.empty()) {
            return input;
        }
        cout << "[Error] Input cannot be empty." << endl;
    }
}

void printHallsAndMenus(const Hall halls[], const MenuPackage menus[]) {
    cout << "\n==================== MARRIAGE HALLS (10 VENUES) ====================" << endl;
    for (int i = 0; i < MAX_HALLS; i++) {
        cout << "Hall #" << halls[i].id << " : " << halls[i].name << " [Sector " << halls[i].area << "]" << endl;
        cout << "  Max Capacity : " << halls[i].capacity << " Guests  |  Base Price : " << formatMoney(halls[i].basePrice) << endl;
        cout << "  Open Hours   : " << halls[i].hours << " (10% discount if guests > 80% capacity)" << endl;
        cout << "--------------------------------------------------------------------" << endl;
    }

    cout << "\n===================== CATERING MENU PACKAGES =======================" << endl;
    cout << "Note: Flat Weekend Surcharge of " << formatMoney(WEEKEND_SURCHARGE) << " on Sat/Sun." << endl;
    for (int i = 0; i < MAX_MENUS; i++) {
        cout << "\n" << menus[i].name << " - " << formatMoney(menus[i].pricePerGuest) << " / guest" << endl;
        for (int j = 0; j < MAX_MENU_ITEMS; j++) {
            cout << "   * " << menus[i].items[j] << endl;
        }
    }
}

void consoleSearchHalls(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings) {
    cout << "\n================ SEARCH HALLS BY SECTOR & FILTERS ==================" << endl;
    cout << "Available Sectors: F-7, E-11, G-11, F-10, I-8, F-8, G-9" << endl;

    string sector;
    while (true) {
        sector = toUpperString(readLineSafe("Enter Islamabad Sector (e.g., F-7): "));
        if (isKnownSector(sector)) {
            break;
        }
        cout << "[Error] Invalid sector! Example valid sectors: F-7, E-11, G-11, F-10, I-8, F-8, G-9." << endl;
    }

    int guests = readIntSafe("Enter Number of Guests (1-1000): ", 1, 1000);
    double budget = readDoubleSafe("Enter Max Hall Base Rent Budget (Rs.): ", 50000.0, 5000000.0);
    int year = readIntSafe("Event Year (2026-2035): ", MIN_YEAR, MAX_YEAR);
    int month = readIntSafe("Event Month (1-12): ", 1, 12);
    int maxDay = daysInMonth(month, year);
    int day = readIntSafe("Event Day (1-" + to_string(maxDay) + "): ", 1, maxDay);
    int slot = readIntSafe("Preferred Time Slot (0=Any, 1=Morning, 2=Evening, 3=Night): ", 0, MAX_SLOTS);

    int rankedIndices[MAX_HALLS];
    int matchScores[MAX_HALLS];
    bool exactMatches[MAX_HALLS];
    rankHallsByFilters(halls, bookings, totalBookings, sector, guests, budget, day, month, year, slot,
        rankedIndices, matchScores, exactMatches);

    cout << "\nSorted Results (Bubble Sort by Match Score) for " << formatDate(day, month, year) << ":" << endl;
    cout << "--------------------------------------------------------------------" << endl;
    for (int i = 0; i < MAX_HALLS; i++) {
        int idx = rankedIndices[i];
        cout << (i + 1) << ". [Hall #" << halls[idx].id << "] " << halls[idx].name
            << " (" << halls[idx].area << ") - "
            << (exactMatches[i] ? "EXACT MATCH" : "Score " + to_string(matchScores[i]) + "/100") << endl;
        cout << "   Capacity: " << halls[idx].capacity << " | Base Rent: " << formatMoney(halls[idx].basePrice) << endl;
        cout << "   Available Timings: ";
        for (int s = 1; s <= MAX_SLOTS; s++) {
            bool avail = isSlotAvailable(bookings, totalBookings, halls[idx].id, day, month, year, s, -1);
            cout << getSlotName(s) << (avail ? " [OPEN] " : " [BOOKED] ");
        }
        cout << "\n--------------------------------------------------------------------" << endl;
    }

    int chosenHall = readIntSafe("Enter Hall ID to book (1-10) or 0 to return: ", 0, MAX_HALLS);
    if (chosenHall > 0) {
        int chosenSlot = (slot == 0) ? readIntSafe("Choose Time Slot (1=Morning, 2=Evening, 3=Night): ", 1, MAX_SLOTS) : slot;
        consoleAddBooking(halls, menus, bookings, totalBookings, chosenHall, day, month, year, chosenSlot, guests);
    }
}

void printAvailabilityMatrix(const Hall halls[], const Booking bookings[], int totalBookings) {
    cout << "\n==================== SLOT AVAILABILITY MATRIX ======================" << endl;
    int year = readIntSafe("Enter Year (2026-2035): ", MIN_YEAR, MAX_YEAR);
    int month = readIntSafe("Enter Month (1-12): ", 1, 12);
    int maxDay = daysInMonth(month, year);
    int day = readIntSafe("Enter Day (1-" + to_string(maxDay) + "): ", 1, maxDay);

    bool bookedSlots[MAX_HALLS][MAX_SLOTS];
    int bookingIdMatrix[MAX_HALLS][MAX_SLOTS];
    int availableCount = 0;
    int bookedCount = 0;

    buildAvailabilityMatrix(bookings, totalBookings, day, month, year,
        bookedSlots, bookingIdMatrix, availableCount, bookedCount);

    cout << "\nDate: " << formatDate(day, month, year)
        << (isWeekendDate(day, month, year) ? " [WEEKEND +Rs. 25,000]" : " [Weekday]") << endl;
    cout << "Summary: " << availableCount << " Available / " << bookedCount << " Booked" << endl;
    cout << "--------------------------------------------------------------------" << endl;

    for (int h = 0; h < MAX_HALLS; h++) {
        cout << "#" << halls[h].id << " " << halls[h].name << " [" << halls[h].area << "] (Max " << halls[h].capacity << ", " << formatMoney(halls[h].basePrice) << "):" << endl;
        for (int s = 0; s < MAX_SLOTS; s++) {
            cout << "   " << getSlotName(s + 1) << " (" << getSlotTimeRange(s + 1) << "): ";
            if (bookedSlots[h][s]) {
                cout << "BOOKED (#" << bookingIdMatrix[h][s] << ")";
            }
            else {
                cout << "AVAILABLE";
            }
            cout << endl;
        }
        cout << "--------------------------------------------------------------------" << endl;
    }
}

void printReceipt(const Booking& booking, const Hall halls[], const MenuPackage menus[]) {
    int hallIdx = findHallIndexById(halls, booking.hallId);
    int menuIdx = findMenuIndexById(menus, booking.menuId);
    string hallName = (hallIdx != -1) ? halls[hallIdx].name : booking.hallName;
    string hallArea = (hallIdx != -1) ? halls[hallIdx].area : "";
    string menuName = (menuIdx != -1) ? menus[menuIdx].name : "Unknown";
    double rate = (menuIdx != -1) ? menus[menuIdx].pricePerGuest : 0.0;

    cout << "\n========================== BOOKING RECEIPT =========================" << endl;
    cout << "  Booking ID        : #" << booking.id << endl;
    cout << "  Client Name       : " << booking.clientName << endl;
    cout << "  Phone Number      : " << booking.clientPhone << endl;
    cout << "  Hall              : " << hallName << " (Sector " << hallArea << ")" << endl;
    cout << "  Event Date        : " << formatDate(booking.day, booking.month, booking.year)
        << (isWeekendDate(booking.day, booking.month, booking.year) ? " (Weekend)" : "") << endl;
    cout << "  Time Slot         : " << getSlotName(booking.slot) << " (" << getSlotTimeRange(booking.slot) << ")" << endl;
    cout << "  Theme & Occasion  : " << booking.theme << " | " << booking.occasion << endl;
    cout << "  Complimentary Perk: " << booking.perks << endl;
    if (!booking.customRequest.empty()) {
        cout << "  Special Request   : " << booking.customRequest << " [" << booking.customRequestStatus << "]" << endl;
    }
    cout << "  Guests            : " << booking.guests << endl;
    cout << "  Catering Menu     : " << menuName << endl;
    cout << "--------------------------------------------------------------------" << endl;
    cout << "  Hall Base Price   : " << formatMoney(booking.hallBasePrice) << endl;
    cout << "  Catering (" << booking.guests << " x " << (long long)rate << ") : " << formatMoney(booking.cateringCost) << endl;
    cout << "  Weekend Surcharge : " << formatMoney(booking.weekendSurcharge) << endl;
    cout << "  10% Bulk Discount : -" << formatMoney(booking.discountAmount) << endl;
    cout << "  TOTAL BILL        : " << formatMoney(booking.totalCost) << endl;
    cout << "====================================================================" << endl;
}

void printBookingsTable(const Booking bookings[], const int indices[], int count, const Hall halls[], const MenuPackage menus[]) {
    cout << "\n======================== YOUR RESERVATIONS =========================" << endl;
    if (count == 0) {
        cout << "No matching bookings found." << endl;
        return;
    }

    double grandTotal = 0.0;
    for (int i = 0; i < count; i++) {
        const Booking& b = bookings[indices[i]];
        int hallIdx = findHallIndexById(halls, b.hallId);
        int menuIdx = findMenuIndexById(menus, b.menuId);
        grandTotal += b.totalCost;

        cout << "#" << b.id << " | " << b.clientName << " (" << b.clientPhone << ")" << endl;
        cout << "   Date: " << formatDate(b.day, b.month, b.year)
            << (isWeekendDate(b.day, b.month, b.year) ? " [Weekend]" : "")
            << " | Slot: " << getSlotName(b.slot)
            << " | Hall: " << (hallIdx != -1 ? halls[hallIdx].name : b.hallName) << endl;
        cout << "   Occasion: " << b.occasion << " | Theme: " << b.theme
            << " | Guests: " << b.guests
            << " | Menu: " << (menuIdx != -1 ? menus[menuIdx].shortName : "?") << endl;
        cout << "   Discount: " << formatMoney(b.discountAmount) << " | Total: " << formatMoney(b.totalCost) << endl;
        cout << "--------------------------------------------------------------------" << endl;
    }
    cout << "Reservations Count: " << count << " | Total Amount: " << formatMoney(grandTotal) << endl;
}

void consoleAddBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings, int preselectedHallId, int preselectedDay, int preselectedMonth, int preselectedYear, int preselectedSlot, int preselectedGuests) {
    cout << "\n========================== BOOK A HALL =============================" << endl;

    int hallId = preselectedHallId;
    if (hallId <= 0) {
        for (int i = 0; i < MAX_HALLS; i++) {
            cout << "  [" << halls[i].id << "] " << halls[i].name << " (" << halls[i].area
                << ", Max " << halls[i].capacity << ", " << formatMoney(halls[i].basePrice) << ")" << endl;
        }
        hallId = readIntSafe("Select Hall (1-10): ", 1, MAX_HALLS);
    }
    int hallIdx = findHallIndexById(halls, hallId);

    int year = (preselectedYear >= MIN_YEAR && preselectedYear <= MAX_YEAR)
        ? preselectedYear : readIntSafe("Event Year (2026-2035): ", MIN_YEAR, MAX_YEAR);
    int month = (preselectedMonth >= 1 && preselectedMonth <= 12)
        ? preselectedMonth : readIntSafe("Event Month (1-12): ", 1, 12);
    int maxDay = daysInMonth(month, year);
    int day = (preselectedDay >= 1 && preselectedDay <= maxDay)
        ? preselectedDay : readIntSafe("Event Day (1-" + to_string(maxDay) + "): ", 1, maxDay);
    int slot = (preselectedSlot >= 1 && preselectedSlot <= MAX_SLOTS)
        ? preselectedSlot : readIntSafe("Select Time Slot (1=Morning, 2=Evening, 3=Night): ", 1, MAX_SLOTS);
    int guests = (preselectedGuests > 0 && preselectedGuests <= halls[hallIdx].capacity)
        ? preselectedGuests : readIntSafe("Number of Guests (1-" + to_string(halls[hallIdx].capacity) + "): ", 1, halls[hallIdx].capacity);

    cout << "Themes: [1] Modern  [2] Floral  [3] Royal" << endl;
    int themeChoice = readIntSafe("Select Theme (1-3): ", 1, 3);
    string theme = (themeChoice == 2) ? "Floral" : (themeChoice == 3 ? "Royal" : "Modern");

    cout << "Occasions: [1] Wedding  [2] Birthday  [3] Event (Seminar/Expo)  [4] Other" << endl;
    int occChoice = readIntSafe("Select Occasion (1-4): ", 1, 4);
    string occasion = "Wedding";
    if (occChoice == 2) occasion = "Birthday";
    else if (occChoice == 3) occasion = "Event";
    else if (occChoice == 4) occasion = readLineSafe("Enter Custom Occasion: ");

    cout << "[Free Perk Applied] " << getOccasionPerks(occasion) << endl;
    cout << "Optional Separate Request (press Enter to skip): ";
    string customRequest;
    getline(cin, customRequest);
    customRequest = sanitizeText(customRequest);

    cout << "Catering Menus:" << endl;
    for (int i = 0; i < MAX_MENUS; i++) {
        cout << "  [" << menus[i].id << "] " << menus[i].name
            << " (" << formatMoney(menus[i].pricePerGuest) << "/guest)" << endl;
    }
    int menuId = readIntSafe("Select Menu Package (1-3): ", 1, MAX_MENUS);

    string name = readLineSafe("Client Name: ");
    string phone;
    while (true) {
        phone = readLineSafe("Phone Number (10-15 digits, optional leading +): ");
        if (isValidPhone(phone)) {
            break;
        }
        cout << "[Error] Invalid phone number format!" << endl;
    }

    Booking created;
    string errorMsg;
    if (addBooking(halls, menus, bookings, totalBookings, name, phone,
        day, month, year, slot, hallId, guests, menuId, theme, occasion, customRequest, created, errorMsg)) {
        cout << "\n[Success] Booking confirmed and saved to " << BOOKINGS_FILE << "!" << endl;
        printReceipt(created, halls, menus);
    }
    else {
        cout << "\n[Error] " << errorMsg << endl;
    }
}

void consoleSearchBookings(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings) {
    cout << "\n====================== SEARCH RESERVATIONS =========================" << endl;
    cout << "Enter Booking ID, Client Name, or Occasion (leave blank for all): ";
    string query;
    getline(cin, query);

    int hallFilter = readIntSafe("Filter by Hall ID (0 for All Halls, 1-10): ", 0, MAX_HALLS);

    int indices[MAX_BOOKINGS];
    int count = searchAndFilterBookings(bookings, totalBookings, query, hallFilter, indices);
    printBookingsTable(bookings, indices, count, halls, menus);
}

void consoleSortBookings(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings) {
    cout << "\n======================= SORT RESERVATIONS ==========================" << endl;
    cout << "  [1] Sort by Date (Ascending)" << endl;
    cout << "  [2] Sort by Date (Descending)" << endl;
    cout << "  [3] Sort by Cost (Low to High)" << endl;
    cout << "  [4] Sort by Cost (High to Low)" << endl;
    int choice = readIntSafe("Select sort option (1-4): ", 1, 4);

    int indices[MAX_BOOKINGS];
    int count = searchAndFilterBookings(bookings, totalBookings, "", 0, indices);

    switch (choice) {
    case 1: sortBookingIndices(bookings, indices, count, "date", "asc"); break;
    case 2: sortBookingIndices(bookings, indices, count, "date", "desc"); break;
    case 3: sortBookingIndices(bookings, indices, count, "cost", "asc"); break;
    case 4: sortBookingIndices(bookings, indices, count, "cost", "desc"); break;
    }

    printBookingsTable(bookings, indices, count, halls, menus);
}

void consoleViewReceipt(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings) {
    int id = readIntSafe("Enter Booking ID for receipt (e.g., 101): ", 1, 999999);
    int idx = findBookingIndexById(bookings, totalBookings, id);
    if (idx == -1) {
        cout << "[Error] Booking #" << id << " not found." << endl;
        return;
    }
    printReceipt(bookings[idx], halls, menus);
}

void consoleUpdateBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int totalBookings) {
    int id = readIntSafe("Enter Booking ID to update: ", 1, 999999);
    int idx = findBookingIndexById(bookings, totalBookings, id);
    if (idx == -1) {
        cout << "[Error] Booking #" << id << " not found." << endl;
        return;
    }

    cout << "Updating Booking #" << id << " (" << bookings[idx].clientName << "):" << endl;
    string name = readLineSafe("New Client Name: ");

    string phone;
    while (true) {
        phone = readLineSafe("New Phone Number (10-15 digits, optional +): ");
        if (isValidPhone(phone)) {
            break;
        }
        cout << "[Error] Invalid phone number format!" << endl;
    }

    int year = readIntSafe("New Event Year (2026-2035): ", MIN_YEAR, MAX_YEAR);
    int month = readIntSafe("New Event Month (1-12): ", 1, 12);
    int maxDay = daysInMonth(month, year);
    int day = readIntSafe("New Event Day (1-" + to_string(maxDay) + "): ", 1, maxDay);
    int slot = readIntSafe("New Time Slot (1=Morning, 2=Evening, 3=Night): ", 1, MAX_SLOTS);
    int hallId = readIntSafe("New Hall ID (1-10): ", 1, MAX_HALLS);
    int hallIdx = findHallIndexById(halls, hallId);
    int guests = readIntSafe("New Guest Count (1-" + to_string(halls[hallIdx].capacity) + "): ", 1, halls[hallIdx].capacity);
    int menuId = readIntSafe("New Menu (1=Basic, 2=Standard, 3=Royal): ", 1, MAX_MENUS);
    int themeChoice = readIntSafe("New Theme (1=Modern, 2=Floral, 3=Royal): ", 1, 3);
    string theme = (themeChoice == 2) ? "Floral" : (themeChoice == 3 ? "Royal" : "Modern");
    string occasion = readLineSafe("New Occasion (Wedding/Birthday/Event/Other): ");

    cout << "New Custom Request (press Enter to keep empty): ";
    string customRequest;
    getline(cin, customRequest);

    Booking updated;
    string errorMsg;
    if (updateBooking(halls, menus, bookings, totalBookings, id, name, phone,
        day, month, year, slot, hallId, guests, menuId, theme, occasion, customRequest,
        bookings[idx].customRequestStatus, updated, errorMsg)) {
        cout << "\n[Success] Booking #" << id << " updated and saved!" << endl;
        printReceipt(updated, halls, menus);
    }
    else {
        cout << "\n[Error] " << errorMsg << endl;
    }
}

void consoleDeleteBooking(Booking bookings[], int& totalBookings) {
    int id = readIntSafe("Enter Booking ID to delete: ", 1, 999999);
    string errorMsg;
    if (deleteBooking(bookings, totalBookings, id, errorMsg)) {
        cout << "[Success] Booking #" << id << " deleted and saved to file." << endl;
    }
    else {
        cout << "[Error] " << errorMsg << endl;
    }
}

void runConsoleMenu(Hall halls[], MenuPackage menus[], Booking bookings[], int& totalBookings) {
    int choice = 0;
    do {
        cout << "\n======================================================" << endl;
        cout << "        MARRIAGE HALL BOOKING SYSTEM (CONSOLE)        " << endl;
        cout << "======================================================" << endl;
        cout << "  [1] View All 10 Halls & Catering Menus" << endl;
        cout << "  [2] Search Halls by Sector & Filters (Bubble Sort)" << endl;
        cout << "  [3] Check Slot Availability Matrix" << endl;
        cout << "  [4] Book a Hall (Add Booking)" << endl;
        cout << "  [5] View All Reservations" << endl;
        cout << "  [6] Search & Filter Reservations" << endl;
        cout << "  [7] Sort Reservations (by Date / Cost)" << endl;
        cout << "  [8] View Booking Receipt" << endl;
        cout << "  [9] Update Booking" << endl;
        cout << "  [10] Delete Booking" << endl;
        cout << "  [0] Return to Main Mode Menu" << endl;
        cout << "------------------------------------------------------" << endl;

        choice = readIntSafe("Enter choice (0-10): ", 0, 10);

        switch (choice) {
        case 1:
            printHallsAndMenus(halls, menus);
            break;
        case 2:
            consoleSearchHalls(halls, menus, bookings, totalBookings);
            break;
        case 3:
            printAvailabilityMatrix(halls, bookings, totalBookings);
            break;
        case 4:
            consoleAddBooking(halls, menus, bookings, totalBookings, 0, 0, 0, 0, 0, 0);
            break;
        case 5: {
            int indices[MAX_BOOKINGS];
            int count = searchAndFilterBookings(bookings, totalBookings, "", 0, indices);
            printBookingsTable(bookings, indices, count, halls, menus);
            break;
        }
        case 6:
            consoleSearchBookings(halls, menus, bookings, totalBookings);
            break;
        case 7:
            consoleSortBookings(halls, menus, bookings, totalBookings);
            break;
        case 8:
            consoleViewReceipt(halls, menus, bookings, totalBookings);
            break;
        case 9:
            consoleUpdateBooking(halls, menus, bookings, totalBookings);
            break;
        case 10:
            consoleDeleteBooking(bookings, totalBookings);
            break;
        case 0:
            cout << "Returning to main mode selection..." << endl;
            break;
        }
    } while (choice != 0);
}
