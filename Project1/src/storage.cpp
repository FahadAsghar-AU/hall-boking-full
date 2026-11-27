#include <fstream>
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

// ==================== STORAGE PROTOTYPES ====================
bool saveBookings(const Booking bookings[], int totalBookings, string filename);
void loadBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[], string filename);

// ==================== FILE HANDLING ====================
bool saveBookings(const Booking bookings[], int totalBookings, string filename) {
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cout << "[Error] Could not open " << filename << " for saving." << endl;
        return false;
    }

    for (int i = 0; i < totalBookings; i++) {
        outFile << bookings[i].id << "|"
            << sanitizeText(bookings[i].clientName) << "|"
            << sanitizeText(bookings[i].clientPhone) << "|"
            << bookings[i].hallId << "|"
            << sanitizeText(bookings[i].hallName) << "|"
            << formatDate(bookings[i].day, bookings[i].month, bookings[i].year) << "|"
            << getSlotName(bookings[i].slot) << "|"
            << sanitizeText(bookings[i].occasion) << "|"
            << sanitizeText(bookings[i].theme) << "|"
            << bookings[i].guests << "|"
            << bookings[i].menuId << "|"
            << sanitizeText(bookings[i].perks) << "|"
            << sanitizeText(bookings[i].customRequest) << "|"
            << sanitizeText(bookings[i].customRequestStatus) << "|"
            << (long long)bookings[i].discountAmount << "|"
            << (long long)bookings[i].totalCost << "\n";
    }

    outFile.close();
    return true;
}

void loadBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[], string filename) {
    totalBookings = 0;
    ifstream inFile(filename);

    if (!inFile.is_open()) {
        seedDefaultBookings(bookings, totalBookings, halls, menus);
        saveBookings(bookings, totalBookings, filename);
        return;
    }

    string line;
    while (getline(inFile, line)) {
        if (sanitizeText(line).empty()) {
            continue;
        }
        if (totalBookings >= MAX_BOOKINGS) {
            break;
        }

        string fields[16];
        int fieldIdx = 0;
        string current = "";

        for (int i = 0; i < (int)line.length(); i++) {
            if (line[i] == '|' && fieldIdx < 15) {
                fields[fieldIdx] = current;
                fieldIdx++;
                current = "";
            }
            else if (line[i] != '\r') {
                current += line[i];
            }
        }
        fields[fieldIdx] = current;

        if (fieldIdx < 15) {
            continue;
        }

        Booking b;
        b.id = parseIntSafe(fields[0], 0);
        b.clientName = sanitizeText(fields[1]);
        b.clientPhone = sanitizeText(fields[2]);
        b.hallId = parseIntSafe(fields[3], 0);
        b.hallName = sanitizeText(fields[4]);

        // Parse DD/MM/YYYY from fields[5]
        string dateStr = sanitizeText(fields[5]);
        b.day = 0;
        b.month = 0;
        b.year = 0;
        if (dateStr.length() == 10 && dateStr[2] == '/' && dateStr[5] == '/') {
            b.day = parseIntSafe(dateStr.substr(0, 2), 0);
            b.month = parseIntSafe(dateStr.substr(3, 2), 0);
            b.year = parseIntSafe(dateStr.substr(6, 4), 0);
        }

        string slotStr = toLowerString(sanitizeText(fields[6]));
        if (slotStr == "morning") b.slot = 1;
        else if (slotStr == "evening") b.slot = 2;
        else if (slotStr == "night") b.slot = 3;
        else b.slot = parseIntSafe(fields[6], 0);

        b.occasion = sanitizeText(fields[7]);
        b.theme = normalizeTheme(fields[8]);
        b.guests = parseIntSafe(fields[9], 0);
        b.menuId = parseIntSafe(fields[10], 0);
        b.perks = sanitizeText(fields[11]);
        b.customRequest = sanitizeText(fields[12]);
        b.customRequestStatus = sanitizeText(fields[13]);

        int hallIdx = findHallIndexById(halls, b.hallId);
        if (hallIdx != -1) {
            b.hallName = halls[hallIdx].name;
        }

        if (b.id <= 0 || !isValidDate(b.day, b.month, b.year) ||
            hallIdx == -1 || findMenuIndexById(menus, b.menuId) == -1) {
            continue;
        }

        calculateBookingCost(halls, menus, b.hallId, b.menuId, b.guests,
            b.day, b.month, b.year, b.hallBasePrice, b.cateringCost,
            b.weekendSurcharge, b.discountAmount, b.totalCost);

        bookings[totalBookings] = b;
        totalBookings++;
    }

    inFile.close();
}

