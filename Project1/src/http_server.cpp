#include "../httplib.h"
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

