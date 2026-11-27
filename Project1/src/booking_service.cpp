#include <utility>
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

// ==================== STORAGE PROTOTYPES ====================
bool saveBookings(const Booking bookings[], int totalBookings, string filename);
void loadBookings(Booking bookings[], int& totalBookings, const Hall halls[], const MenuPackage menus[], string filename);

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

    SeedRow seeds[2] = {
        {101, "Ayesha & Bilal Khan",      "+923001234567", 15, 11, 2026, 2, 1, 250, 3, "Royal",  "Wedding", "Extra floral stage backdrop", "Accepted"},
        {102, "Fatima & Usman Tariq",     "+923219876543", 15, 11, 2026, 3, 3, 450, 2, "Floral", "Wedding", "", "None"}
    };

    for (int i = 0; i < 2; i++) {
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

