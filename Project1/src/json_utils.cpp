#include <cctype>
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

// ==================== JSON PROTOTYPES ====================
string escapeJson(string text);
int findJsonKeyValuePos(string body, string key);
string extractJsonString(string body, string key);
int extractJsonInt(string body, string key, int defaultValue);
string hallsToJson(const Hall halls[]);
string hallSearchResultsToJson(const Hall halls[], const Booking bookings[], int totalBookings, string desiredSector, int guests, double maxBudget, int day, int month, int year, int desiredSlot);
string menusToJson(const MenuPackage menus[]);
string availabilityToJson(const Hall halls[], const Booking bookings[], int totalBookings, int day, int month, int year);
string bookingToJson(const Booking& booking, const Hall halls[], const MenuPackage menus[]);
string bookingsListToJson(const Booking bookings[], int totalBookings, const int indices[], int count, const Hall halls[], const MenuPackage menus[]);

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

