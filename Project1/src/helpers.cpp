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

