#include "httplib.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <utility> // for swap()
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
const double BULK_DISCOUNT_RATE = 0.10; // 10% discount when guests > 80% of hall capacity
const int SERVER_PORT = 8080;
const string BOOKINGS_FILE = "bookings.txt";

// ==================== DATA STRUCTURES ====================
struct Hall {
    int id;
    string name;
    string area;       // Islamabad sector (e.g., F-7, E-11)
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
    int slot;                    // 1 = Morning, 2 = Evening, 3 = Night
    string theme;                // Modern, Floral, Royal
    string occasion;             // Wedding, Birthday, Event, or custom occasion
    string perks;                // Complimentary perks based on occasion
    string customRequest;        // Optional user request
    string customRequestStatus;  // None, Pending, Accepted, Declined
    int guests;
    int menuId;
    double hallBasePrice;
    double cateringCost;
    double weekendSurcharge;
    double discountAmount;
    double totalCost;
};

// ==================== FUNCTION PROTOTYPES ====================
// Initialization & Seeding
void initHalls(Hall halls[]);
void initMenus(MenuPackage menus[]);
void seedDefaultBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[]);

// Date, Validation & String Helpers
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

// Sector, Theme, Occasion, Perks & Lookup Helpers
bool isKnownSector(string sector);
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

// Core Business Logic (Search, Bubble Sort, Availability, Pricing, CRUD)
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

// File Handling (Save & Load)
bool saveBookings(const Booking bookings[], int totalBookings, string filename);
void loadBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[], string filename);

// JSON Serialization & Parsing (No extra headers)
string escapeJson(string text);
string extractJsonString(string body, string key);
int extractJsonInt(string body, string key, int defaultValue);
string hallsToJson(const Hall halls[]);
string hallSearchResultsToJson(const Hall halls[], const Booking bookings[], int totalBookings, string desiredSector, int guests, double maxBudget, int day, int month, int year, int desiredSlot);
string menusToJson(const MenuPackage menus[]);
string availabilityToJson(const Hall halls[], const Booking bookings[], int totalBookings, int day, int month, int year);
string bookingToJson(const Booking& booking, const Hall halls[], const MenuPackage menus[]);
string bookingsListToJson(const Booking bookings[], int totalBookings, const int indices[], int count, const Hall halls[], const MenuPackage menus[]);

// HTTP Server (httplib)
void setCorsHeaders(httplib::Response& res);
void startHttpServer(Hall halls[], MenuPackage menus[], Booking bookings[], int& totalBookings);

// Console Menu Mode (PDF Proposal Requirement)
int readIntSafe(string prompt, int minVal, int maxVal);
double readDoubleSafe(string prompt, double minVal, double maxVal);
string readLineSafe(string prompt);
void printHallsAndMenus(const Hall halls[], const MenuPackage menus[]);
void consoleSearchHalls(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings);
void printAvailabilityMatrix(const Hall halls[], const Booking bookings[], int totalBookings);
void printReceipt(const Booking& booking, const Hall halls[], const MenuPackage menus[]);
void printBookingsTable(const Booking bookings[], const int indices[], int count, const Hall halls[], const MenuPackage menus[]);
void consoleAddBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings, int preselectedHallId, int preselectedDay, int preselectedMonth, int preselectedYear, int preselectedSlot, int preselectedGuests);
void consoleSearchBookings(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings);
void consoleSortBookings(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings);
void consoleViewReceipt(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings);
void consoleUpdateBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int totalBookings);
void consoleDeleteBooking(Booking bookings[], int& totalBookings);
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

// ==================== INITIALIZATION & SEEDING ====================
void initHalls(Hall halls[]) {
    // 10 Hardcoded Halls across Islamabad sectors:
    // 3 sectors (F-7, E-11, G-11) have 2 halls each; remaining 4 sectors have 1 hall each.

    // --- Sector F-7 (2 Halls) ---
    halls[0].id = 1;
    halls[0].name = "Grand Ballroom";
    halls[0].area = "F-7";
    halls[0].hours = "09:00 AM - 01:00 AM";
    halls[0].capacity = 300;
    halls[0].basePrice = 150000.0;

    halls[1].id = 2;
    halls[1].name = "Margalla Vista Marquee";
    halls[1].area = "F-7";
    halls[1].hours = "09:00 AM - 01:00 AM";
    halls[1].capacity = 600;
    halls[1].basePrice = 280000.0;

    // --- Sector E-11 (2 Halls) ---
    halls[2].id = 3;
    halls[2].name = "Royal Palace";
    halls[2].area = "E-11";
    halls[2].hours = "09:00 AM - 01:00 AM";
    halls[2].capacity = 500;
    halls[2].basePrice = 250000.0;

    halls[3].id = 4;
    halls[3].name = "Crystal Banquet Hall";
    halls[3].area = "E-11";
    halls[3].hours = "09:00 AM - 01:00 AM";
    halls[3].capacity = 400;
    halls[3].basePrice = 200000.0;

    // --- Sector G-11 (2 Halls) ---
    halls[4].id = 5;
    halls[4].name = "Imperial Arena";
    halls[4].area = "G-11";
    halls[4].hours = "09:00 AM - 01:00 AM";
    halls[4].capacity = 800;
    halls[4].basePrice = 400000.0;

    halls[5].id = 6;
    halls[5].name = "Pearl Marquee";
    halls[5].area = "G-11";
    halls[5].hours = "09:00 AM - 01:00 AM";
    halls[5].capacity = 250;
    halls[5].basePrice = 130000.0;

    // --- Sector F-10 (1 Hall) ---
    halls[6].id = 7;
    halls[6].name = "Emerald Gardens";
    halls[6].area = "F-10";
    halls[6].hours = "09:00 AM - 01:00 AM";
    halls[6].capacity = 450;
    halls[6].basePrice = 220000.0;

    // --- Sector I-8 (1 Hall) ---
    halls[7].id = 8;
    halls[7].name = "Capital Convention Center";
    halls[7].area = "I-8";
    halls[7].hours = "09:00 AM - 01:00 AM";
    halls[7].capacity = 1000;
    halls[7].basePrice = 500000.0;

    // --- Sector F-8 (1 Hall) ---
    halls[8].id = 9;
    halls[8].name = "Crown Heritage Hall";
    halls[8].area = "F-8";
    halls[8].hours = "09:00 AM - 01:00 AM";
    halls[8].capacity = 350;
    halls[8].basePrice = 180000.0;

    // --- Sector G-9 (1 Hall) ---
    halls[9].id = 10;
    halls[9].name = "Starlight Event Studio";
    halls[9].area = "G-9";
    halls[9].hours = "09:00 AM - 01:00 AM";
    halls[9].capacity = 200;
    halls[9].basePrice = 110000.0;
}

void initMenus(MenuPackage menus[]) {
    menus[0].id = 1;
    menus[0].shortName = "Menu 1";
    menus[0].name = "Menu 1: Basic Banqueting";
    menus[0].pricePerGuest = 1200.0;
    menus[0].items[0] = "Chicken Qorma / Karachi Biryani";
    menus[0].items[1] = "Fresh Roghani Naan";
    menus[0].items[2] = "Raita & Mint Chutney";
    menus[0].items[3] = "Fresh Green Salad";
    menus[0].items[4] = "Traditional Shahi Kheer";
    menus[0].items[5] = "Mineral Water & Soft Drinks";

    menus[1].id = 2;
    menus[1].shortName = "Menu 2";
    menus[1].name = "Menu 2: Standard Elegance";
    menus[1].pricePerGuest = 1800.0;
    menus[1].items[0] = "Mutton Yakhni Pulao";
    menus[1].items[1] = "Chicken Karahi (Desi Ghee)";
    menus[1].items[2] = "Chicken Seekh Kababs";
    menus[1].items[3] = "Tandoori Roghani Naan";
    menus[1].items[4] = "Gulab Jamun with Rabri";
    menus[1].items[5] = "Assorted Soft Drinks & Tea";

    menus[2].id = 3;
    menus[2].shortName = "Menu 3";
    menus[2].name = "Menu 3: Royal Imperial";
    menus[2].pricePerGuest = 2500.0;
    menus[2].items[0] = "Mutton Champ Roast";
    menus[2].items[1] = "Imperial Mutton Biryani";
    menus[2].items[2] = "Reshmi Kababs & Fish Tikka";
    menus[2].items[3] = "Taftan & Garlic Naan";
    menus[2].items[4] = "Live Kulfa / Assorted Desserts";
    menus[2].items[5] = "Welcome Mocktails & Green Tea";
}

void seedDefaultBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[]) {
    totalBookings = 0;

    struct SeedRow {
        int id;
        string name;
        string phone;
        int day;
        int month;
        int year;
        int slot;
        int hallId;
        int guests;
        int menuId;
        string theme;
        string occasion;
        string customRequest;
        string requestStatus;
    };

    SeedRow seeds[5] = {
        {101, "Ayesha & Bilal Khan",      "+923001234567", 15, 11, 2026, 2, 1, 250, 3, "Royal",  "Wedding", "Extra floral stage backdrop", "Accepted"},
        {102, "Fatima & Usman Tariq",     "+923219876543", 15, 11, 2026, 3, 3, 450, 2, "Floral", "Wedding", "", "None"},
        {103, "Zainab & Hamza Malik",     "+923334567890", 20, 11, 2026, 2, 3, 400, 3, "Royal",  "Wedding", "", "None"},
        {104, "Sana & Ahmed Raza",        "+923455678901", 22, 11, 2026, 1, 5, 700, 2, "Modern", "Event",   "2 cordless lapel mics for speakers", "Pending"},
        {105, "Maryam & Daniyal Siddiqui","+923126789012", 28, 11, 2026, 3, 1, 280, 1, "Floral", "Birthday","", "None"}
    };

    for (int i = 0; i < 5; i++) {
        Booking b;
        b.id = seeds[i].id;
        b.clientName = seeds[i].name;
        b.clientPhone = seeds[i].phone;
        b.day = seeds[i].day;
        b.month = seeds[i].month;
        b.year = seeds[i].year;
        b.slot = seeds[i].slot;
        b.hallId = seeds[i].hallId;
        int hallIdx = findHallIndexById(halls, b.hallId);
        b.hallName = (hallIdx != -1) ? halls[hallIdx].name : "Unknown";
        b.guests = seeds[i].guests;
        b.menuId = seeds[i].menuId;
        b.theme = seeds[i].theme;
        b.occasion = seeds[i].occasion;
        b.perks = getOccasionPerks(b.occasion);
        b.customRequest = seeds[i].customRequest;
        b.customRequestStatus = seeds[i].requestStatus;

        calculateBookingCost(halls, menus, b.hallId, b.menuId, b.guests,
            b.day, b.month, b.year, b.hallBasePrice, b.cateringCost,
            b.weekendSurcharge, b.discountAmount, b.totalCost);

        bookings[totalBookings] = b;
        totalBookings++;
    }
}

// ==================== DATE, VALIDATION & STRING HELPERS ====================
bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int month, int year) {
    switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        return 31;
    case 4: case 6: case 9: case 11:
        return 30;
    case 2:
        return isLeapYear(year) ? 29 : 28;
    default:
        return 0;
    }
}

bool isValidDate(int day, int month, int year) {
    if (year < MIN_YEAR || year > MAX_YEAR) {
        return false;
    }
    if (month < 1 || month > 12) {
        return false;
    }
    int maxDays = daysInMonth(month, year);
    return (day >= 1 && day <= maxDays);
}

int getDayOfWeek(int day, int month, int year) {
    int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) {
        year -= 1;
    }
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

bool isWeekendDate(int day, int month, int year) {
    if (!isValidDate(day, month, year)) {
        return false;
    }
    int dow = getDayOfWeek(day, month, year);
    return (dow == 0 || dow == 6);
}

bool isValidPhone(string phone) {
    string trimmed = sanitizeText(phone);
    if (trimmed.empty()) {
        return false;
    }

    int startIdx = 0;
    if (trimmed[0] == '+') {
        startIdx = 1;
    }

    int digitCount = 0;
    for (int i = startIdx; i < (int)trimmed.length(); i++) {
        if (!isdigit((unsigned char)trimmed[i])) {
            return false;
        }
        digitCount++;
    }

    return (digitCount >= 10 && digitCount <= 15);
}

string sanitizeText(string text) {
    int start = 0;
    while (start < (int)text.length() && isspace((unsigned char)text[start])) {
        start++;
    }
    int end = (int)text.length() - 1;
    while (end >= start && isspace((unsigned char)text[end])) {
        end--;
    }

    string cleaned = "";
    for (int i = start; i <= end; i++) {
        if (text[i] == '|') {
            cleaned += ' ';
        }
        else if (text[i] != '\r' && text[i] != '\n') {
            cleaned += text[i];
        }
    }
    return cleaned;
}

string toLowerString(string text) {
    for (int i = 0; i < (int)text.length(); i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
    return text;
}

string toUpperString(string text) {
    for (int i = 0; i < (int)text.length(); i++) {
        text[i] = (char)toupper((unsigned char)text[i]);
    }
    return text;
}

int parseIntSafe(string text, int defaultValue) {
    string s = sanitizeText(text);
    if (s.empty()) {
        return defaultValue;
    }

    int idx = 0;
    bool negative = false;
    if (s[0] == '-') {
        negative = true;
        idx = 1;
    }
    else if (s[0] == '+') {
        idx = 1;
    }

    if (idx >= (int)s.length()) {
        return defaultValue;
    }

    long long value = 0;
    for (int i = idx; i < (int)s.length(); i++) {
        if (!isdigit((unsigned char)s[i])) {
            return defaultValue;
        }
        value = value * 10 + (s[i] - '0');
        if (value > 2000000000LL) {
            return defaultValue;
        }
    }

    return negative ? (int)(-value) : (int)value;
}

double parseDoubleSafe(string text, double defaultValue) {
    string s = sanitizeText(text);
    if (s.empty()) {
        return defaultValue;
    }
    bool seenDot = false;
    double value = 0.0;
    double fracFactor = 0.1;
    int start = 0;
    bool negative = false;
    if (s[0] == '-') {
        negative = true;
        start = 1;
    }
    if (start >= (int)s.length()) {
        return defaultValue;
    }
    for (int i = start; i < (int)s.length(); i++) {
        if (s[i] == '.' && !seenDot) {
            seenDot = true;
        }
        else if (isdigit((unsigned char)s[i])) {
            int digit = s[i] - '0';
            if (!seenDot) {
                value = value * 10.0 + digit;
            }
            else {
                value += digit * fracFactor;
                fracFactor *= 0.1;
            }
        }
        else {
            return defaultValue;
        }
    }
    return negative ? -value : value;
}

// ==================== SECTOR, THEME, OCCASION & PERKS HELPERS ====================
bool isKnownSector(string sector) {
    string s = toUpperString(sanitizeText(sector));
    if (s.empty()) {
        return false;
    }
    // Accept Islamabad sector pattern like F-7, E-11, G-10, I-8, D-12, H-9, etc.
    int dashPos = (int)s.find('-');
    if (dashPos != 1 || s.length() < 3 || s.length() > 4) {
        return false;
    }
    char zone = s[0];
    if (zone < 'D' || zone > 'I') {
        return false;
    }
    for (int i = 2; i < (int)s.length(); i++) {
        if (!isdigit((unsigned char)s[i])) {
            return false;
        }
    }
    int num = parseIntSafe(s.substr(2), -1);
    return (num >= 5 && num <= 18);
}

int extractSectorNumber(string sector) {
    int dashPos = (int)sector.find('-');
    if (dashPos == -1 || dashPos + 1 >= (int)sector.length()) {
        return -1;
    }
    return parseIntSafe(sector.substr(dashPos + 1), -1);
}

int computeSectorMatchScore(string hallSector, string desiredSector) {
    string h = toUpperString(sanitizeText(hallSector));
    string d = toUpperString(sanitizeText(desiredSector));
    if (d.empty()) {
        return 20; // No sector filter specified
    }
    if (h == d) {
        return 45; // Exact sector match
    }

    int score = 0;
    if (!h.empty() && !d.empty() && h[0] == d[0]) {
        score += 18; // Same Islamabad sector series (e.g. F-series)
    }

    int hNum = extractSectorNumber(h);
    int dNum = extractSectorNumber(d);
    if (hNum != -1 && dNum != -1) {
        int diff = hNum - dNum;
        if (diff < 0) diff = -diff;
        if (diff <= 1) {
            score += 12;
        }
        else if (diff <= 2) {
            score += 8;
        }
        else if (diff <= 4) {
            score += 4;
        }
    }
    return score;
}

bool isValidTheme(string theme) {
    string t = toLowerString(sanitizeText(theme));
    return (t == "modern" || t == "floral" || t == "royal");
}

string normalizeTheme(string theme) {
    string t = toLowerString(sanitizeText(theme));
    if (t == "floral") return "Floral";
    if (t == "royal") return "Royal";
    return "Modern";
}

string getOccasionPerks(string occasion) {
    string occ = toLowerString(sanitizeText(occasion));
    if (occ == "birthday" || occ.find("birth") != string::npos) {
        return "Free Dancing Gorilla & Mascot + Sound System & DJ";
    }
    if (occ == "wedding" || occ.find("nikkah") != string::npos || occ.find("walima") != string::npos || occ.find("mehndi") != string::npos) {
        return "Free Sound System & Live DJ + Bridal Stage Lighting";
    }
    if (occ == "event" || occ.find("seminar") != string::npos || occ.find("expo") != string::npos || occ.find("conference") != string::npos) {
        return "Free Dedicated Management Staff + Presentation Audio";
    }
    return "Free Standard Sound System";
}

// ==================== LOOKUP & FORMATTING HELPERS ====================
int findHallIndexById(const Hall halls[], int hallId) {
    for (int i = 0; i < MAX_HALLS; i++) {
        if (halls[i].id == hallId) {
            return i;
        }
    }
    return -1;
}

int findMenuIndexById(const MenuPackage menus[], int menuId) {
    for (int i = 0; i < MAX_MENUS; i++) {
        if (menus[i].id == menuId) {
            return i;
        }
    }
    return -1;
}

int findBookingIndexById(const Booking bookings[], int totalBookings, int bookingId) {
    for (int i = 0; i < totalBookings; i++) {
        if (bookings[i].id == bookingId) {
            return i;
        }
    }
    return -1;
}

string getSlotName(int slot) {
    switch (slot) {
    case 1: return "Morning";
    case 2: return "Evening";
    case 3: return "Night";
    default: return "Unknown";
    }
}

string getSlotTimeRange(int slot) {
    switch (slot) {
    case 1: return "09:00 AM - 02:00 PM";
    case 2: return "03:00 PM - 07:00 PM";
    case 3: return "08:00 PM - 01:00 AM";
    default: return "";
    }
}

string formatDate(int day, int month, int year) {
    string d = (day < 10 ? "0" : "") + to_string(day);
    string m = (month < 10 ? "0" : "") + to_string(month);
    return d + "/" + m + "/" + to_string(year);
}

string formatMoney(double amount) {
    long long val = (long long)(amount + 0.5);
    if (val == 0) {
        return "Rs. 0";
    }
    bool neg = (val < 0);
    if (neg) val = -val;
    string digits = to_string(val);
    string formatted = "";
    int count = 0;
    for (int i = (int)digits.length() - 1; i >= 0; i--) {
        formatted = digits[i] + formatted;
        count++;
        if (count % 3 == 0 && i > 0) {
            formatted = "," + formatted;
        }
    }
    return (neg ? "-Rs. " : "Rs. ") + formatted;
}

// ==================== CORE BUSINESS LOGIC ====================
bool isSlotAvailable(const Booking bookings[], int totalBookings, int hallId, int day, int month, int year, int slot, int ignoreBookingId) {
    for (int i = 0; i < totalBookings; i++) {
        if (bookings[i].id == ignoreBookingId) {
            continue;
        }
        if (bookings[i].hallId == hallId &&
            bookings[i].day == day &&
            bookings[i].month == month &&
            bookings[i].year == year &&
            bookings[i].slot == slot) {
            return false;
        }
    }
    return true;
}

void buildAvailabilityMatrix(const Booking bookings[], int totalBookings, int day, int month, int year, bool bookedSlots[][MAX_SLOTS], int bookingIdMatrix[][MAX_SLOTS], int& availableCount, int& bookedCount) {
    availableCount = 0;
    bookedCount = 0;

    for (int h = 0; h < MAX_HALLS; h++) {
        for (int s = 0; s < MAX_SLOTS; s++) {
            bookedSlots[h][s] = false;
            bookingIdMatrix[h][s] = 0;
        }
    }

    for (int i = 0; i < totalBookings; i++) {
        if (bookings[i].day == day && bookings[i].month == month && bookings[i].year == year) {
            int hallIdx = bookings[i].hallId - 1;
            int slotIdx = bookings[i].slot - 1;
            if (hallIdx >= 0 && hallIdx < MAX_HALLS && slotIdx >= 0 && slotIdx < MAX_SLOTS) {
                bookedSlots[hallIdx][slotIdx] = true;
                bookingIdMatrix[hallIdx][slotIdx] = bookings[i].id;
            }
        }
    }

    for (int h = 0; h < MAX_HALLS; h++) {
        for (int s = 0; s < MAX_SLOTS; s++) {
            if (bookedSlots[h][s]) {
                bookedCount++;
            }
            else {
                availableCount++;
            }
        }
    }
}

void rankHallsByFilters(const Hall halls[], const Booking bookings[], int totalBookings, string desiredSector, int guests, double maxBudget, int day, int month, int year, int desiredSlot, int rankedIndices[], int matchScores[], bool exactMatches[]) {
    string cleanSector = toUpperString(sanitizeText(desiredSector));

    for (int i = 0; i < MAX_HALLS; i++) {
        rankedIndices[i] = i;
        int score = 0;
        bool sectorExact = cleanSector.empty() || (halls[i].area == cleanSector);
        bool capacityOk = (guests <= 0) || (halls[i].capacity >= guests);
        bool budgetOk = (maxBudget <= 0.0) || (halls[i].basePrice <= maxBudget);

        bool slotOk = false;
        if (desiredSlot >= 1 && desiredSlot <= MAX_SLOTS) {
            slotOk = isSlotAvailable(bookings, totalBookings, halls[i].id, day, month, year, desiredSlot, -1);
        }
        else {
            // If "Any Slot" (0), available if at least 1 slot is open on that date
            for (int s = 1; s <= MAX_SLOTS; s++) {
                if (isSlotAvailable(bookings, totalBookings, halls[i].id, day, month, year, s, -1)) {
                    slotOk = true;
                    break;
                }
            }
        }

        score += computeSectorMatchScore(halls[i].area, cleanSector);

        if (capacityOk) {
            score += 25;
        }
        else if (halls[i].capacity >= (guests * 7) / 10) {
            score += 8;
        }

        if (budgetOk) {
            score += 20;
        }
        else if (maxBudget > 0.0 && halls[i].basePrice <= maxBudget * 1.25) {
            score += 6;
        }

        if (slotOk) {
            score += 15;
        }

        matchScores[i] = score;
        exactMatches[i] = (sectorExact && capacityOk && budgetOk && slotOk);
    }

    // Bubble Sort descending by exactMatch first, then matchScore, then lower basePrice
    for (int i = 0; i < MAX_HALLS - 1; i++) {
        for (int j = 0; j < MAX_HALLS - i - 1; j++) {
            bool shouldSwap = false;
            if (exactMatches[j] != exactMatches[j + 1]) {
                if (!exactMatches[j] && exactMatches[j + 1]) {
                    shouldSwap = true;
                }
            }
            else if (matchScores[j] < matchScores[j + 1]) {
                shouldSwap = true;
            }
            else if (matchScores[j] == matchScores[j + 1]) {
                int idxA = rankedIndices[j];
                int idxB = rankedIndices[j + 1];
                if (halls[idxA].basePrice > halls[idxB].basePrice) {
                    shouldSwap = true;
                }
            }

            if (shouldSwap) {
                swap(matchScores[j], matchScores[j + 1]);
                swap(rankedIndices[j], rankedIndices[j + 1]);
                swap(exactMatches[j], exactMatches[j + 1]);
            }
        }
    }
}

bool qualifiesForBulkDiscount(int guests, int hallCapacity) {
    // 10% discount when guests are strictly more than 80% of the hall's guest capacity
    return (guests * 100 > hallCapacity * 80);
}

bool calculateBookingCost(const Hall halls[], const MenuPackage menus[], int hallId, int menuId, int guests, int day, int month, int year, double& hallBasePrice, double& cateringCost, double& weekendSurcharge, double& discountAmount, double& totalCost) {
    int hallIdx = findHallIndexById(halls, hallId);
    int menuIdx = findMenuIndexById(menus, menuId);
    if (hallIdx == -1 || menuIdx == -1) {
        return false;
    }

    hallBasePrice = halls[hallIdx].basePrice;
    cateringCost = (double)guests * menus[menuIdx].pricePerGuest;
    weekendSurcharge = isWeekendDate(day, month, year) ? WEEKEND_SURCHARGE : 0.0;

    double subtotal = hallBasePrice + cateringCost + weekendSurcharge;
    if (guests > 0 && guests <= halls[hallIdx].capacity && qualifiesForBulkDiscount(guests, halls[hallIdx].capacity)) {
        discountAmount = (double)((long long)(subtotal * BULK_DISCOUNT_RATE + 0.5));
    }
    else {
        discountAmount = 0.0;
    }

    totalCost = subtotal - discountAmount;
    return true;
}

bool validateBookingInput(const Hall halls[], const MenuPackage menus[], const Booking bookings[], int totalBookings, string clientName, string clientPhone, int day, int month, int year, int slot, int hallId, int guests, int menuId, string theme, string occasion, int ignoreBookingId, string& errorMsg) {
    if (sanitizeText(clientName).empty()) {
        errorMsg = "Client name is required.";
        return false;
    }
    if (!isValidPhone(clientPhone)) {
        errorMsg = "Phone number must contain 10-15 digits (optional leading +).";
        return false;
    }
    if (!isValidDate(day, month, year)) {
        errorMsg = "Invalid event date. Allowed year range is 2026-2035.";
        return false;
    }
    if (slot < 1 || slot > MAX_SLOTS) {
        errorMsg = "Invalid time slot. Choose 1 (Morning), 2 (Evening), or 3 (Night).";
        return false;
    }

    int hallIdx = findHallIndexById(halls, hallId);
    if (hallIdx == -1) {
        errorMsg = "Invalid hall selected.";
        return false;
    }
    if (guests <= 0 || guests > halls[hallIdx].capacity) {
        errorMsg = "Guest count must be between 1 and " + to_string(halls[hallIdx].capacity) + " for " + halls[hallIdx].name + ".";
        return false;
    }

    int menuIdx = findMenuIndexById(menus, menuId);
    if (menuIdx == -1) {
        errorMsg = "Invalid catering menu package selected.";
        return false;
    }

    if (!isValidTheme(theme)) {
        errorMsg = "Invalid theme. Choose Modern, Floral, or Royal.";
        return false;
    }

    if (sanitizeText(occasion).empty()) {
        errorMsg = "Please select or enter an occasion.";
        return false;
    }

    if (!isSlotAvailable(bookings, totalBookings, hallId, day, month, year, slot, ignoreBookingId)) {
        errorMsg = "Selected hall and time slot are already booked on " + formatDate(day, month, year) + ".";
        return false;
    }

    errorMsg = "";
    return true;
}

int getNextBookingId(const Booking bookings[], int totalBookings) {
    int maxId = 100;
    for (int i = 0; i < totalBookings; i++) {
        if (bookings[i].id > maxId) {
            maxId = bookings[i].id;
        }
    }
    return maxId + 1;
}

bool addBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int& totalBookings, string clientName, string clientPhone, int day, int month, int year, int slot, int hallId, int guests, int menuId, string theme, string occasion, string customRequest, Booking& createdBooking, string& errorMsg) {
    if (totalBookings >= MAX_BOOKINGS) {
        errorMsg = "Booking storage limit reached (max " + to_string(MAX_BOOKINGS) + " bookings).";
        return false;
    }

    string cleanName = sanitizeText(clientName);
    string cleanPhone = sanitizeText(clientPhone);
    string cleanOccasion = sanitizeText(occasion);
    string cleanRequest = sanitizeText(customRequest);

    if (!validateBookingInput(halls, menus, bookings, totalBookings, cleanName, cleanPhone,
        day, month, year, slot, hallId, guests, menuId, theme, cleanOccasion, -1, errorMsg)) {
        return false;
    }

    int hallIdx = findHallIndexById(halls, hallId);
    createdBooking.id = getNextBookingId(bookings, totalBookings);
    createdBooking.clientName = cleanName;
    createdBooking.clientPhone = cleanPhone;
    createdBooking.hallId = hallId;
    createdBooking.hallName = halls[hallIdx].name;
    createdBooking.day = day;
    createdBooking.month = month;
    createdBooking.year = year;
    createdBooking.slot = slot;
    createdBooking.theme = normalizeTheme(theme);
    createdBooking.occasion = cleanOccasion;
    createdBooking.perks = getOccasionPerks(cleanOccasion);
    createdBooking.customRequest = cleanRequest;
    createdBooking.customRequestStatus = cleanRequest.empty() ? "None" : "Pending";
    createdBooking.guests = guests;
    createdBooking.menuId = menuId;

    calculateBookingCost(halls, menus, hallId, menuId, guests, day, month, year,
        createdBooking.hallBasePrice, createdBooking.cateringCost,
        createdBooking.weekendSurcharge, createdBooking.discountAmount, createdBooking.totalCost);

    bookings[totalBookings] = createdBooking;
    totalBookings++;

    saveBookings(bookings, totalBookings, BOOKINGS_FILE);
    return true;
}

bool updateBooking(const Hall halls[], const MenuPackage menus[], Booking bookings[], int totalBookings, int bookingId, string clientName, string clientPhone, int day, int month, int year, int slot, int hallId, int guests, int menuId, string theme, string occasion, string customRequest, string requestStatus, Booking& updatedBooking, string& errorMsg) {
    int idx = findBookingIndexById(bookings, totalBookings, bookingId);
    if (idx == -1) {
        errorMsg = "Booking #" + to_string(bookingId) + " not found.";
        return false;
    }

    string cleanName = sanitizeText(clientName);
    string cleanPhone = sanitizeText(clientPhone);
    string cleanOccasion = sanitizeText(occasion);
    string cleanRequest = sanitizeText(customRequest);

    if (!validateBookingInput(halls, menus, bookings, totalBookings, cleanName, cleanPhone,
        day, month, year, slot, hallId, guests, menuId, theme, cleanOccasion, bookingId, errorMsg)) {
        return false;
    }

    int hallIdx = findHallIndexById(halls, hallId);
    bookings[idx].clientName = cleanName;
    bookings[idx].clientPhone = cleanPhone;
    bookings[idx].hallId = hallId;
    bookings[idx].hallName = halls[hallIdx].name;
    bookings[idx].day = day;
    bookings[idx].month = month;
    bookings[idx].year = year;
    bookings[idx].slot = slot;
    bookings[idx].theme = normalizeTheme(theme);
    bookings[idx].occasion = cleanOccasion;
    bookings[idx].perks = getOccasionPerks(cleanOccasion);
    bookings[idx].customRequest = cleanRequest;

    if (cleanRequest.empty()) {
        bookings[idx].customRequestStatus = "None";
    }
    else {
        string cleanStatus = sanitizeText(requestStatus);
        if (cleanStatus == "Accepted" || cleanStatus == "Declined" || cleanStatus == "Pending") {
            bookings[idx].customRequestStatus = cleanStatus;
        }
        else if (bookings[idx].customRequestStatus == "None") {
            bookings[idx].customRequestStatus = "Pending";
        }
    }

    bookings[idx].guests = guests;
    bookings[idx].menuId = menuId;

    calculateBookingCost(halls, menus, hallId, menuId, guests, day, month, year,
        bookings[idx].hallBasePrice, bookings[idx].cateringCost,
        bookings[idx].weekendSurcharge, bookings[idx].discountAmount, bookings[idx].totalCost);

    updatedBooking = bookings[idx];
    saveBookings(bookings, totalBookings, BOOKINGS_FILE);
    return true;
}

bool deleteBooking(Booking bookings[], int& totalBookings, int bookingId, string& errorMsg) {
    int idx = findBookingIndexById(bookings, totalBookings, bookingId);
    if (idx == -1) {
        errorMsg = "Booking #" + to_string(bookingId) + " not found.";
        return false;
    }

    for (int i = idx; i < totalBookings - 1; i++) {
        bookings[i] = bookings[i + 1];
    }
    totalBookings--;

    saveBookings(bookings, totalBookings, BOOKINGS_FILE);
    return true;
}

int searchAndFilterBookings(const Booking bookings[], int totalBookings, string searchQuery, int hallFilterId, int resultIndices[]) {
    string queryLower = toLowerString(sanitizeText(searchQuery));
    if (!queryLower.empty() && queryLower[0] == '#') {
        queryLower = queryLower.substr(1);
    }

    int count = 0;
    for (int i = 0; i < totalBookings; i++) {
        if (hallFilterId > 0 && bookings[i].hallId != hallFilterId) {
            continue;
        }

        if (!queryLower.empty()) {
            string idStr = to_string(bookings[i].id);
            string nameLower = toLowerString(bookings[i].clientName);
            string phoneLower = toLowerString(bookings[i].clientPhone);
            string occasionLower = toLowerString(bookings[i].occasion);

            bool matchesId = (idStr.find(queryLower) != string::npos);
            bool matchesName = (nameLower.find(queryLower) != string::npos);
            bool matchesPhone = (phoneLower.find(queryLower) != string::npos);
            bool matchesOccasion = (occasionLower.find(queryLower) != string::npos);

            if (!matchesId && !matchesName && !matchesPhone && !matchesOccasion) {
                continue;
            }
        }

        resultIndices[count] = i;
        count++;
    }
    return count;
}

void sortBookingIndices(const Booking bookings[], int indices[], int count, string sortBy, string order) {
    bool descending = (toLowerString(order) == "desc");
    string mode = toLowerString(sortBy);

    // Bubble Sort on booking index array
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            const Booking& a = bookings[indices[j]];
            const Booking& b = bookings[indices[j + 1]];
            bool shouldSwap = false;

            if (mode == "cost") {
                shouldSwap = descending ? (a.totalCost < b.totalCost) : (a.totalCost > b.totalCost);
            }
            else if (mode == "date") {
                long long keyA = (long long)a.year * 100000LL + (long long)a.month * 1000LL + (long long)a.day * 10LL + a.slot;
                long long keyB = (long long)b.year * 100000LL + (long long)b.month * 1000LL + (long long)b.day * 10LL + b.slot;
                shouldSwap = descending ? (keyA < keyB) : (keyA > keyB);
            }
            else {
                shouldSwap = descending ? (a.id < b.id) : (a.id > b.id);
            }

            if (shouldSwap) {
                swap(indices[j], indices[j + 1]);
            }
        }
    }
}

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

// ==================== JSON HELPERS (NO EXTRA HEADERS) ====================
string escapeJson(string text) {
    string out = "";
    for (int i = 0; i < (int)text.length(); i++) {
        char c = text[i];
        if (c == '"') {
            out += "\\\"";
        }
        else if (c == '\\') {
            out += "\\\\";
        }
        else if (c == '\n') {
            out += "\\n";
        }
        else if (c == '\r') {
            out += "\\r";
        }
        else if (c == '\t') {
            out += "\\t";
        }
        else if ((unsigned char)c >= 32) {
            out += c;
        }
    }
    return out;
}

int findJsonKeyValuePos(string body, string key) {
    string pattern = "\"" + key + "\"";
    int pos = 0;

    while (true) {
        size_t found = body.find(pattern, pos);
        if (found == string::npos) {
            return -1;
        }
        int idx = (int)found + (int)pattern.length();
        while (idx < (int)body.length() && isspace((unsigned char)body[idx])) {
            idx++;
        }
        if (idx < (int)body.length() && body[idx] == ':') {
            idx++;
            while (idx < (int)body.length() && isspace((unsigned char)body[idx])) {
                idx++;
            }
            return idx;
        }
        pos = (int)found + 1;
    }
}

string extractJsonString(string body, string key) {
    int idx = findJsonKeyValuePos(body, key);
    if (idx == -1 || idx >= (int)body.length() || body[idx] != '"') {
        return "";
    }

    idx++;
    string result = "";
    while (idx < (int)body.length()) {
        char c = body[idx];
        if (c == '\\' && idx + 1 < (int)body.length()) {
            char next = body[idx + 1];
            if (next == '"' || next == '\\' || next == '/') {
                result += next;
            }
            else if (next == 'n') {
                result += '\n';
            }
            else if (next == 't') {
                result += '\t';
            }
            idx += 2;
            continue;
        }
        if (c == '"') {
            break;
        }
        result += c;
        idx++;
    }
    return result;
}

int extractJsonInt(string body, string key, int defaultValue) {
    int idx = findJsonKeyValuePos(body, key);
    if (idx == -1 || idx >= (int)body.length()) {
        return defaultValue;
    }

    bool quoted = false;
    if (body[idx] == '"') {
        quoted = true;
        idx++;
    }

    string numStr = "";
    if (idx < (int)body.length() && body[idx] == '-') {
        numStr += '-';
        idx++;
    }
    while (idx < (int)body.length() && isdigit((unsigned char)body[idx])) {
        numStr += body[idx];
        idx++;
    }

    if (quoted && idx < (int)body.length() && body[idx] != '"') {
        return defaultValue;
    }

    return parseIntSafe(numStr, defaultValue);
}

string hallsToJson(const Hall halls[]) {
    string json = "[";
    for (int i = 0; i < MAX_HALLS; i++) {
        if (i > 0) {
            json += ",";
        }
        json += "{";
        json += "\"id\":" + to_string(halls[i].id) + ",";
        json += "\"name\":\"" + escapeJson(halls[i].name) + "\",";
        json += "\"area\":\"" + escapeJson(halls[i].area) + "\",";
        json += "\"hours\":\"" + escapeJson(halls[i].hours) + "\",";
        json += "\"capacity\":" + to_string(halls[i].capacity) + ",";
        json += "\"basePrice\":" + to_string((long long)halls[i].basePrice);
        json += "}";
    }
    json += "]";
    return json;
}

string hallSearchResultsToJson(const Hall halls[], const Booking bookings[], int totalBookings, string desiredSector, int guests, double maxBudget, int day, int month, int year, int desiredSlot) {
    int rankedIndices[MAX_HALLS];
    int matchScores[MAX_HALLS];
    bool exactMatches[MAX_HALLS];

    rankHallsByFilters(halls, bookings, totalBookings, desiredSector, guests, maxBudget,
        day, month, year, desiredSlot, rankedIndices, matchScores, exactMatches);

    string cleanSector = toUpperString(sanitizeText(desiredSector));
    bool weekend = isWeekendDate(day, month, year);

    string json = "{";
    json += "\"sector\":\"" + escapeJson(cleanSector) + "\",";
    json += "\"guests\":" + to_string(guests) + ",";
    json += "\"budget\":" + to_string((long long)maxBudget) + ",";
    json += "\"day\":" + to_string(day) + ",";
    json += "\"month\":" + to_string(month) + ",";
    json += "\"year\":" + to_string(year) + ",";
    json += "\"dateFormatted\":\"" + formatDate(day, month, year) + "\",";
    json += "\"isWeekend\":" + string(weekend ? "true" : "false") + ",";
    json += "\"results\":[";

    for (int i = 0; i < MAX_HALLS; i++) {
        int idx = rankedIndices[i];
        if (i > 0) {
            json += ",";
        }

        string matchLabel = "Partial Match";
        if (exactMatches[i]) {
            matchLabel = "Exact Match";
        }
        else if (!cleanSector.empty() && halls[idx].area == cleanSector) {
            matchLabel = "Sector Match";
        }
        else if (matchScores[i] >= 55) {
            matchLabel = "Close Match";
        }

        json += "{";
        json += "\"id\":" + to_string(halls[idx].id) + ",";
        json += "\"name\":\"" + escapeJson(halls[idx].name) + "\",";
        json += "\"area\":\"" + escapeJson(halls[idx].area) + "\",";
        json += "\"hours\":\"" + escapeJson(halls[idx].hours) + "\",";
        json += "\"capacity\":" + to_string(halls[idx].capacity) + ",";
        json += "\"basePrice\":" + to_string((long long)halls[idx].basePrice) + ",";
        json += "\"matchScore\":" + to_string(matchScores[i]) + ",";
        json += "\"exactMatch\":" + string(exactMatches[i] ? "true" : "false") + ",";
        json += "\"matchLabel\":\"" + matchLabel + "\",";
        json += "\"qualifiesDiscount\":" + string((guests > 0 && guests <= halls[idx].capacity && qualifiesForBulkDiscount(guests, halls[idx].capacity)) ? "true" : "false") + ",";
        json += "\"availableSlots\":[";
        for (int s = 1; s <= MAX_SLOTS; s++) {
            if (s > 1) {
                json += ",";
            }
            bool avail = isSlotAvailable(bookings, totalBookings, halls[idx].id, day, month, year, s, -1);
            json += "{";
            json += "\"slot\":" + to_string(s) + ",";
            json += "\"slotName\":\"" + getSlotName(s) + "\",";
            json += "\"timeRange\":\"" + getSlotTimeRange(s) + "\",";
            json += "\"available\":" + string(avail ? "true" : "false");
            json += "}";
        }
        json += "]}";
    }

    json += "]}";
    return json;
}

string menusToJson(const MenuPackage menus[]) {
    string json = "{\"weekendSurcharge\":" + to_string((long long)WEEKEND_SURCHARGE) + ",\"discountThresholdPercent\":80,\"discountPercent\":10,\"menus\":[";
    for (int i = 0; i < MAX_MENUS; i++) {
        if (i > 0) {
            json += ",";
        }
        json += "{";
        json += "\"id\":" + to_string(menus[i].id) + ",";
        json += "\"shortName\":\"" + escapeJson(menus[i].shortName) + "\",";
        json += "\"name\":\"" + escapeJson(menus[i].name) + "\",";
        json += "\"pricePerGuest\":" + to_string((long long)menus[i].pricePerGuest) + ",";
        json += "\"items\":[";
        for (int j = 0; j < MAX_MENU_ITEMS; j++) {
            if (j > 0) {
                json += ",";
            }
            json += "\"" + escapeJson(menus[i].items[j]) + "\"";
        }
        json += "]}";
    }
    json += "]}";
    return json;
}

string availabilityToJson(const Hall halls[], const Booking bookings[], int totalBookings, int day, int month, int year) {
    bool bookedSlots[MAX_HALLS][MAX_SLOTS];
    int bookingIdMatrix[MAX_HALLS][MAX_SLOTS];
    int availableCount = 0;
    int bookedCount = 0;

    buildAvailabilityMatrix(bookings, totalBookings, day, month, year,
        bookedSlots, bookingIdMatrix, availableCount, bookedCount);

    bool weekend = isWeekendDate(day, month, year);

    string json = "{";
    json += "\"day\":" + to_string(day) + ",";
    json += "\"month\":" + to_string(month) + ",";
    json += "\"year\":" + to_string(year) + ",";
    json += "\"dateFormatted\":\"" + formatDate(day, month, year) + "\",";
    json += "\"isWeekend\":" + string(weekend ? "true" : "false") + ",";
    json += "\"weekendSurcharge\":" + to_string(weekend ? (long long)WEEKEND_SURCHARGE : 0LL) + ",";
    json += "\"availableCount\":" + to_string(availableCount) + ",";
    json += "\"bookedCount\":" + to_string(bookedCount) + ",";
    json += "\"halls\":[";

    for (int h = 0; h < MAX_HALLS; h++) {
        if (h > 0) {
            json += ",";
        }
        json += "{";
        json += "\"hallId\":" + to_string(halls[h].id) + ",";
        json += "\"hallName\":\"" + escapeJson(halls[h].name) + "\",";
        json += "\"area\":\"" + escapeJson(halls[h].area) + "\",";
        json += "\"capacity\":" + to_string(halls[h].capacity) + ",";
        json += "\"basePrice\":" + to_string((long long)halls[h].basePrice) + ",";
        json += "\"slots\":[";

        for (int s = 0; s < MAX_SLOTS; s++) {
            if (s > 0) {
                json += ",";
            }
            int slotNum = s + 1;
            bool available = !bookedSlots[h][s];
            json += "{";
            json += "\"slot\":" + to_string(slotNum) + ",";
            json += "\"slotName\":\"" + getSlotName(slotNum) + "\",";
            json += "\"timeRange\":\"" + getSlotTimeRange(slotNum) + "\",";
            json += "\"available\":" + string(available ? "true" : "false") + ",";
            json += "\"bookingId\":" + to_string(bookingIdMatrix[h][s]);
            json += "}";
        }
        json += "]}";
    }
    json += "]}";
    return json;
}

string bookingToJson(const Booking& booking, const Hall halls[], const MenuPackage menus[]) {
    int hallIdx = findHallIndexById(halls, booking.hallId);
    int menuIdx = findMenuIndexById(menus, booking.menuId);
    string hallName = (hallIdx != -1) ? halls[hallIdx].name : booking.hallName;
    string hallArea = (hallIdx != -1) ? halls[hallIdx].area : "";
    string menuName = (menuIdx != -1) ? menus[menuIdx].name : "Unknown";
    string menuShort = (menuIdx != -1) ? menus[menuIdx].shortName : "Unknown";
    double pricePerGuest = (menuIdx != -1) ? menus[menuIdx].pricePerGuest : 0.0;
    bool weekend = isWeekendDate(booking.day, booking.month, booking.year);

    string json = "{";
    json += "\"id\":" + to_string(booking.id) + ",";
    json += "\"clientName\":\"" + escapeJson(booking.clientName) + "\",";
    json += "\"clientPhone\":\"" + escapeJson(booking.clientPhone) + "\",";
    json += "\"day\":" + to_string(booking.day) + ",";
    json += "\"month\":" + to_string(booking.month) + ",";
    json += "\"year\":" + to_string(booking.year) + ",";
    json += "\"dateFormatted\":\"" + formatDate(booking.day, booking.month, booking.year) + "\",";
    json += "\"isWeekend\":" + string(weekend ? "true" : "false") + ",";
    json += "\"slot\":" + to_string(booking.slot) + ",";
    json += "\"slotName\":\"" + getSlotName(booking.slot) + "\",";
    json += "\"slotTime\":\"" + getSlotTimeRange(booking.slot) + "\",";
    json += "\"hallId\":" + to_string(booking.hallId) + ",";
    json += "\"hallName\":\"" + escapeJson(hallName) + "\",";
    json += "\"hallArea\":\"" + escapeJson(hallArea) + "\",";
    json += "\"theme\":\"" + escapeJson(booking.theme) + "\",";
    json += "\"occasion\":\"" + escapeJson(booking.occasion) + "\",";
    json += "\"perks\":\"" + escapeJson(booking.perks) + "\",";
    json += "\"customRequest\":\"" + escapeJson(booking.customRequest) + "\",";
    json += "\"customRequestStatus\":\"" + escapeJson(booking.customRequestStatus) + "\",";
    json += "\"guests\":" + to_string(booking.guests) + ",";
    json += "\"menuId\":" + to_string(booking.menuId) + ",";
    json += "\"menuShortName\":\"" + escapeJson(menuShort) + "\",";
    json += "\"menuName\":\"" + escapeJson(menuName) + "\",";
    json += "\"pricePerGuest\":" + to_string((long long)pricePerGuest) + ",";
    json += "\"hallBasePrice\":" + to_string((long long)booking.hallBasePrice) + ",";
    json += "\"cateringSubtotal\":" + to_string((long long)booking.cateringCost) + ",";
    json += "\"weekendSurcharge\":" + to_string((long long)booking.weekendSurcharge) + ",";
    json += "\"discountAmount\":" + to_string((long long)booking.discountAmount) + ",";
    json += "\"totalCost\":" + to_string((long long)booking.totalCost);
    json += "}";
    return json;
}

string bookingsListToJson(const Booking bookings[], int totalBookings, const int indices[], int count, const Hall halls[], const MenuPackage menus[]) {
    double grandTotal = 0.0;
    string itemsJson = "[";
    for (int i = 0; i < count; i++) {
        if (i > 0) {
            itemsJson += ",";
        }
        int idx = indices[i];
        grandTotal += bookings[idx].totalCost;
        itemsJson += bookingToJson(bookings[idx], halls, menus);
    }
    itemsJson += "]";

    string json = "{";
    json += "\"count\":" + to_string(count) + ",";
    json += "\"totalBookings\":" + to_string(totalBookings) + ",";
    json += "\"totalAmount\":" + to_string((long long)grandTotal) + ",";
    json += "\"bookings\":" + itemsJson;
    json += "}";
    return json;
}

// ==================== HTTP SERVER (HTTPLIB) ====================
void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void startHttpServer(Hall halls[], MenuPackage menus[], Booking bookings[], int& totalBookings) {
    httplib::Server svr;

    // Serve the HTML/CSS/JS frontend folder at http://localhost:8080/ if found
    string frontendCandidates[4] = {"frontend", "../frontend", "../../frontend", "../../../frontend"};
    string mountedFrontend = "";
    for (int i = 0; i < 4; i++) {
        ifstream checkIndex(frontendCandidates[i] + "/index.html");
        if (checkIndex.is_open()) {
            checkIndex.close();
            if (svr.set_mount_point("/", frontendCandidates[i])) {
                mountedFrontend = frontendCandidates[i];
            }
            break;
        }
    }

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.status = 204;
    });

    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.set_content("{\"status\":\"ok\",\"service\":\"Hall Booking System\"}", "application/json");
    });

    svr.Get("/api/halls", [&](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.set_content(hallsToJson(halls), "application/json");
    });

    // Search & Rank 10 Halls using Sector, Guests, Budget, Date & Time Slot (Bubble Sort)
    svr.Get("/api/halls/search", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        string sector = req.has_param("sector") ? sanitizeText(req.get_param_value("sector")) : "";
        int guests = req.has_param("guests") ? parseIntSafe(req.get_param_value("guests"), 0) : 0;
        double budget = req.has_param("budget") ? parseDoubleSafe(req.get_param_value("budget"), 0.0) : 0.0;
        int day = req.has_param("day") ? parseIntSafe(req.get_param_value("day"), 15) : 15;
        int month = req.has_param("month") ? parseIntSafe(req.get_param_value("month"), 11) : 11;
        int year = req.has_param("year") ? parseIntSafe(req.get_param_value("year"), 2026) : 2026;
        int slot = req.has_param("slot") ? parseIntSafe(req.get_param_value("slot"), 0) : 0;

        if (!sector.empty() && !isKnownSector(sector)) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid Islamabad sector format. Use e.g. F-7, E-11, G-11, F-10, I-8, F-8, G-9.\"}", "application/json");
            return;
        }
        if (guests < 0 || guests > 2000) {
            res.status = 400;
            res.set_content("{\"error\":\"Guest count must be a positive number (up to 1000 max hall capacity).\"}", "application/json");
            return;
        }
        if (budget < 0) {
            res.status = 400;
            res.set_content("{\"error\":\"Budget cannot be negative.\"}", "application/json");
            return;
        }
        if (!isValidDate(day, month, year)) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid date. Allowed year range is 2026-2035.\"}", "application/json");
            return;
        }
        if (slot < 0 || slot > MAX_SLOTS) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid time slot.\"}", "application/json");
            return;
        }

        res.set_content(
            hallSearchResultsToJson(halls, bookings, totalBookings, sector, guests, budget, day, month, year, slot),
            "application/json"
        );
    });

    svr.Get("/api/menus", [&](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.set_content(menusToJson(menus), "application/json");
    });

    svr.Get("/api/availability", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        int day = req.has_param("day") ? parseIntSafe(req.get_param_value("day"), 15) : 15;
        int month = req.has_param("month") ? parseIntSafe(req.get_param_value("month"), 11) : 11;
        int year = req.has_param("year") ? parseIntSafe(req.get_param_value("year"), 2026) : 2026;

        if (!isValidDate(day, month, year)) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid date. Allowed year range is 2026-2035.\"}", "application/json");
            return;
        }

        res.set_content(availabilityToJson(halls, bookings, totalBookings, day, month, year), "application/json");
    });

    // Live Cost Breakdown, 10% Bulk Discount (>80% capacity) & Perks Preview
    svr.Post("/api/calculate-cost", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        int day = extractJsonInt(req.body, "day", 0);
        int month = extractJsonInt(req.body, "month", 0);
        int year = extractJsonInt(req.body, "year", 0);
        int slot = extractJsonInt(req.body, "slot", 0);
        int hallId = extractJsonInt(req.body, "hallId", 0);
        int guests = extractJsonInt(req.body, "guests", 0);
        int menuId = extractJsonInt(req.body, "menuId", 0);
        string phone = extractJsonString(req.body, "clientPhone");
        if (phone.empty()) {
            phone = extractJsonString(req.body, "phone");
        }
        string occasion = extractJsonString(req.body, "occasion");
        if (occasion.empty()) {
            occasion = "Wedding";
        }
        int ignoreId = extractJsonInt(req.body, "ignoreBookingId", -1);

        int hallIdx = findHallIndexById(halls, hallId);
        int menuIdx = findMenuIndexById(menus, menuId);
        if (hallIdx == -1 || menuIdx == -1 || !isValidDate(day, month, year)) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid hall, menu, or date.\"}", "application/json");
            return;
        }

        double hallBase = 0.0, catering = 0.0, surcharge = 0.0, discount = 0.0, total = 0.0;
        calculateBookingCost(halls, menus, hallId, menuId, guests > 0 ? guests : 0,
            day, month, year, hallBase, catering, surcharge, discount, total);

        bool phoneOk = isValidPhone(phone);
        bool capacityOk = (guests > 0 && guests <= halls[hallIdx].capacity);
        bool slotOk = (slot >= 1 && slot <= MAX_SLOTS) &&
            isSlotAvailable(bookings, totalBookings, hallId, day, month, year, slot, ignoreId);
        bool weekend = isWeekendDate(day, month, year);
        bool bulkDiscount = (capacityOk && qualifiesForBulkDiscount(guests, halls[hallIdx].capacity));
        string perks = getOccasionPerks(occasion);

        string json = "{";
        json += "\"hallBasePrice\":" + to_string((long long)hallBase) + ",";
        json += "\"cateringSubtotal\":" + to_string((long long)catering) + ",";
        json += "\"weekendSurcharge\":" + to_string((long long)surcharge) + ",";
        json += "\"discountAmount\":" + to_string((long long)discount) + ",";
        json += "\"discountApplied\":" + string(bulkDiscount ? "true" : "false") + ",";
        json += "\"totalCost\":" + to_string((long long)total) + ",";
        json += "\"perks\":\"" + escapeJson(perks) + "\",";
        json += "\"isWeekend\":" + string(weekend ? "true" : "false") + ",";
        json += "\"validPhone\":" + string(phoneOk ? "true" : "false") + ",";
        json += "\"withinCapacity\":" + string(capacityOk ? "true" : "false") + ",";
        json += "\"slotAvailable\":" + string(slotOk ? "true" : "false");
        json += "}";

        res.set_content(json, "application/json");
    });

    svr.Get("/api/bookings", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        string searchQuery = req.has_param("search") ? req.get_param_value("search") : "";
        int hallFilterId = req.has_param("hallId") ? parseIntSafe(req.get_param_value("hallId"), 0) : 0;
        string sortBy = req.has_param("sortBy") ? req.get_param_value("sortBy") : "id";
        string order = req.has_param("order") ? req.get_param_value("order") : "asc";

        int indices[MAX_BOOKINGS];
        int matchCount = searchAndFilterBookings(bookings, totalBookings, searchQuery, hallFilterId, indices);
        sortBookingIndices(bookings, indices, matchCount, sortBy, order);

        res.set_content(bookingsListToJson(bookings, totalBookings, indices, matchCount, halls, menus), "application/json");
    });

    svr.Get("/api/bookings/:id", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        int bookingId = req.path_params.count("id") ? parseIntSafe(req.path_params.at("id"), -1) : -1;
        int idx = findBookingIndexById(bookings, totalBookings, bookingId);
        if (idx == -1) {
            res.status = 404;
            res.set_content("{\"error\":\"Booking not found.\"}", "application/json");
            return;
        }
        res.set_content(bookingToJson(bookings[idx], halls, menus), "application/json");
    });

    svr.Post("/api/bookings", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        string clientName = extractJsonString(req.body, "clientName");
        if (clientName.empty()) clientName = extractJsonString(req.body, "name");
        string clientPhone = extractJsonString(req.body, "clientPhone");
        if (clientPhone.empty()) clientPhone = extractJsonString(req.body, "phone");

        int day = extractJsonInt(req.body, "day", 0);
        int month = extractJsonInt(req.body, "month", 0);
        int year = extractJsonInt(req.body, "year", 0);
        int slot = extractJsonInt(req.body, "slot", 0);
        int hallId = extractJsonInt(req.body, "hallId", 0);
        int guests = extractJsonInt(req.body, "guests", 0);
        int menuId = extractJsonInt(req.body, "menuId", 0);
        string theme = extractJsonString(req.body, "theme");
        if (theme.empty()) theme = "Modern";
        string occasion = extractJsonString(req.body, "occasion");
        string customRequest = extractJsonString(req.body, "customRequest");

        Booking created;
        string errorMsg;
        if (!addBooking(halls, menus, bookings, totalBookings, clientName, clientPhone,
            day, month, year, slot, hallId, guests, menuId, theme, occasion, customRequest, created, errorMsg)) {
            res.status = 400;
            res.set_content("{\"error\":\"" + escapeJson(errorMsg) + "\"}", "application/json");
            return;
        }

        cout << "[HTTP] Created Booking #" << created.id << " for " << created.clientName << endl;
        res.status = 201;
        res.set_content(bookingToJson(created, halls, menus), "application/json");
    });

    svr.Put("/api/bookings/:id", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        int bookingId = req.path_params.count("id") ? parseIntSafe(req.path_params.at("id"), -1) : -1;
        int idx = findBookingIndexById(bookings, totalBookings, bookingId);
        if (idx == -1) {
            res.status = 404;
            res.set_content("{\"error\":\"Booking not found.\"}", "application/json");
            return;
        }

        string clientName = extractJsonString(req.body, "clientName");
        if (clientName.empty()) clientName = extractJsonString(req.body, "name");
        string clientPhone = extractJsonString(req.body, "clientPhone");
        if (clientPhone.empty()) clientPhone = extractJsonString(req.body, "phone");

        int day = extractJsonInt(req.body, "day", 0);
        int month = extractJsonInt(req.body, "month", 0);
        int year = extractJsonInt(req.body, "year", 0);
        int slot = extractJsonInt(req.body, "slot", 0);
        int hallId = extractJsonInt(req.body, "hallId", 0);
        int guests = extractJsonInt(req.body, "guests", 0);
        int menuId = extractJsonInt(req.body, "menuId", 0);
        string theme = extractJsonString(req.body, "theme");
        if (theme.empty()) theme = bookings[idx].theme;
        string occasion = extractJsonString(req.body, "occasion");
        if (occasion.empty()) occasion = bookings[idx].occasion;
        string customRequest = extractJsonString(req.body, "customRequest");
        string requestStatus = extractJsonString(req.body, "customRequestStatus");

        Booking updated;
        string errorMsg;
        if (!updateBooking(halls, menus, bookings, totalBookings, bookingId, clientName, clientPhone,
            day, month, year, slot, hallId, guests, menuId, theme, occasion, customRequest, requestStatus, updated, errorMsg)) {
            res.status = 400;
            res.set_content("{\"error\":\"" + escapeJson(errorMsg) + "\"}", "application/json");
            return;
        }

        cout << "[HTTP] Updated Booking #" << updated.id << endl;
        res.set_content(bookingToJson(updated, halls, menus), "application/json");
    });

    // Accept or Decline a separate custom request
    svr.Patch("/api/bookings/:id/request", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        int bookingId = req.path_params.count("id") ? parseIntSafe(req.path_params.at("id"), -1) : -1;
        int idx = findBookingIndexById(bookings, totalBookings, bookingId);
        if (idx == -1) {
            res.status = 404;
            res.set_content("{\"error\":\"Booking not found.\"}", "application/json");
            return;
        }
        if (bookings[idx].customRequest.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"No custom request attached to this booking.\"}", "application/json");
            return;
        }
        string status = sanitizeText(extractJsonString(req.body, "status"));
        if (status != "Accepted" && status != "Declined" && status != "Pending") {
            res.status = 400;
            res.set_content("{\"error\":\"Status must be Accepted, Declined, or Pending.\"}", "application/json");
            return;
        }
        bookings[idx].customRequestStatus = status;
        saveBookings(bookings, totalBookings, BOOKINGS_FILE);
        res.set_content(bookingToJson(bookings[idx], halls, menus), "application/json");
    });

    svr.Delete("/api/bookings/:id", [&](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        int bookingId = req.path_params.count("id") ? parseIntSafe(req.path_params.at("id"), -1) : -1;
        string errorMsg;
        if (!deleteBooking(bookings, totalBookings, bookingId, errorMsg)) {
            res.status = 404;
            res.set_content("{\"error\":\"" + escapeJson(errorMsg) + "\"}", "application/json");
            return;
        }

        cout << "[HTTP] Deleted Booking #" << bookingId << endl;
        res.set_content("{\"message\":\"Booking deleted successfully.\",\"id\":" + to_string(bookingId) + "}", "application/json");
    });

    cout << "\n======================================================" << endl;
    cout << "  HTTP Server running on http://localhost:" << SERVER_PORT << endl;
    if (!mountedFrontend.empty()) {
        cout << "  Frontend mounted from: " << mountedFrontend << " (open http://localhost:" << SERVER_PORT << ")" << endl;
    }
    cout << "  Ready to connect with Frontend UI!" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << "  Endpoints:" << endl;
    cout << "    GET    /api/health" << endl;
    cout << "    GET    /api/halls" << endl;
    cout << "    GET    /api/halls/search?sector=&guests=&budget=&day=&month=&year=&slot=" << endl;
    cout << "    GET    /api/menus" << endl;
    cout << "    GET    /api/availability?day=15&month=11&year=2026" << endl;
    cout << "    POST   /api/calculate-cost" << endl;
    cout << "    GET    /api/bookings?search=&hallId=0&sortBy=id&order=asc" << endl;
    cout << "    GET    /api/bookings/:id" << endl;
    cout << "    POST   /api/bookings" << endl;
    cout << "    PUT    /api/bookings/:id" << endl;
    cout << "    PATCH  /api/bookings/:id/request" << endl;
    cout << "    DELETE /api/bookings/:id" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << "  Press Ctrl+C to stop the server." << endl;
    cout << "======================================================" << endl;

    if (!svr.listen("0.0.0.0", SERVER_PORT)) {
        cout << "[Error] Failed to bind to port " << SERVER_PORT << ". Is another server already running?" << endl;
    }
}

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
