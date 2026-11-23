#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <utility> // for swap()
using namespace std;

const int MAX_HALLS = 10;
const int MAX_BOOKINGS = 100;

// ==================== PARTNER 1 (HUSSAIN) PROTOTYPES ====================
void initHalls(int hallIds[], string hallNames[], string hallAreas[], string hallThemes[], string hallOccasions[], int hallCapacities[], double hallBasePrices[]);
int searchHalls(string area, int guests, double maxBudget, string occasion, string theme, string hallAreas[], int hallCapacities[], double hallBasePrices[], string hallOccasions[], string hallThemes[], int totalHalls, int matchedIndices[]);
void rankFallbackHalls(string area, int guests, double maxBudget, string occasion, string hallAreas[], int hallCapacities[], double hallBasePrices[], string hallOccasions[], int totalHalls, int rankedIndices[], int matchScores[]);

// ==================== PARTNER 2 PROTOTYPES ====================
// Partner 2 will paste their 11 function prototypes here

int main() {
    // 1. Shared Hall Parallel Arrays (Indices 0..9)
    int hallIds[MAX_HALLS];
    string hallNames[MAX_HALLS];
    string hallAreas[MAX_HALLS];
    string hallThemes[MAX_HALLS];
    string hallOccasions[MAX_HALLS];
    int hallCapacities[MAX_HALLS];
    double hallBasePrices[MAX_HALLS];

    // 2. Shared Booking Parallel Arrays (Indices 0..totalBookings-1)
    int bookingIds[MAX_BOOKINGS];
    string clientNames[MAX_BOOKINGS];
    string clientPhones[MAX_BOOKINGS];
    int bookingDays[MAX_BOOKINGS];
    int bookingMonths[MAX_BOOKINGS];
    int bookingYears[MAX_BOOKINGS];
    int bookingSlots[MAX_BOOKINGS];      // 1 = Morning, 2 = Evening, 3 = Night
    int bookingHallIds[MAX_BOOKINGS];    // 1..10
    int bookingGuests[MAX_BOOKINGS];
    int bookingMenus[MAX_BOOKINGS];      // 1 = Basic, 2 = Standard, 3 = Premium
    int bookingDaysCount[MAX_BOOKINGS];
    double bookingCosts[MAX_BOOKINGS];

    int totalBookings = 0;
    bool bookedSlots[MAX_HALLS][3]; // 2D Availability Matrix (10 Halls x 3 Slots)

    // --- PHASE 1: Initialize 10 Halls ---
    initHalls(hallIds, hallNames, hallAreas, hallThemes, hallOccasions, hallCapacities, hallBasePrices);

    // --- PHASE 2: Interactive Hall Search with Input Validation ---
    string desiredArea, desiredOccasion, desiredTheme;
    int guestCount;
    double maxBudget;

    int matchedIndices[MAX_HALLS];
    int rankedIndices[MAX_HALLS];
    int matchScores[MAX_HALLS];

    cout << "==============================================" << endl;
    cout << "        ISLAMABAD HALL BOOKING SYSTEM         " << endl;
    cout << "==============================================" << endl;
    cout << "Available Sectors : F-7, E-11, G-11, F-10, I-8, F-8, G-9" << endl;
    cout << "Occasion Types    : Wedding, Birthday, Expo" << endl;
    cout << "Decor Themes      : Royal, Floral, Modern" << endl;
    cout << "----------------------------------------------" << endl;

    // 1. Validate Sector
    while (true) {
        cout << "Enter Sector (e.g., F-7): ";
        cin >> desiredArea;
        cin.ignore(10000, '\n');

        for (int i = 0; i < desiredArea.length(); i++) {
            desiredArea[i] = toupper(desiredArea[i]);
        }

        if (desiredArea == "F-7" || desiredArea == "E-11" || desiredArea == "G-11" ||
            desiredArea == "F-10" || desiredArea == "I-8" || desiredArea == "F-8" ||
            desiredArea == "G-9") {
            break;
        }
        cout << "[Error] Invalid sector! Please enter one of: F-7, E-11, G-11, F-10, I-8, F-8, G-9." << endl;
    }

    // 2. Validate Guest Count
    while (true) {
        cout << "Enter Number of Guests: ";
        cin >> guestCount;

        if (cin.fail() || guestCount <= 0) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "[Error] Invalid input! Please enter a positive whole number for guests." << endl;
        }
        else {
            cin.ignore(10000, '\n');
            if (guestCount > 1000) {
                cout << "[Warning] Our largest venue (Capital Convention Center) accommodates up to 1,000 guests." << endl;
            }
            break;
        }
    }

    // 3. Validate Maximum Budget
    while (true) {
        cout << "Enter Maximum Hall Base Rent Budget (Rs.): ";
        cin >> maxBudget;

        if (cin.fail() || maxBudget <= 0) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "[Error] Invalid budget! Please enter a positive number in Rs." << endl;
        }
        else {
            cin.ignore(10000, '\n');
            if (maxBudget < 110000.0) {
                cout << "[Warning] Base rent across our halls starts at Rs. 110,000." << endl;
            }
            break;
        }
    }

    // 4. Validate Occasion
    while (true) {
        cout << "Enter Occasion (Wedding / Birthday / Expo): ";
        cin >> desiredOccasion;
        cin.ignore(10000, '\n');

        desiredOccasion[0] = toupper(desiredOccasion[0]);
        for (int i = 1; i < desiredOccasion.length(); i++) {
            desiredOccasion[i] = tolower(desiredOccasion[i]);
        }

        if (desiredOccasion == "Wedding" || desiredOccasion == "Birthday" || desiredOccasion == "Expo") {
            break;
        }
        cout << "[Error] Invalid occasion! Please enter Wedding, Birthday, or Expo." << endl;
    }

    // 5. Validate Theme
    while (true) {
        cout << "Enter Theme (Royal / Floral / Modern): ";
        cin >> desiredTheme;
        cin.ignore(10000, '\n');

        desiredTheme[0] = toupper(desiredTheme[0]);
        for (int i = 1; i < desiredTheme.length(); i++) {
            desiredTheme[i] = tolower(desiredTheme[i]);
        }

        if (desiredTheme == "Royal" || desiredTheme == "Floral" || desiredTheme == "Modern") {
            break;
        }
        cout << "[Error] Invalid theme! Please enter Royal, Floral, or Modern." << endl;
    }

    // --- Run Search or Fallback Ranking ---
    int exactCount = searchHalls(desiredArea, guestCount, maxBudget, desiredOccasion, desiredTheme,
        hallAreas, hallCapacities, hallBasePrices, hallOccasions, hallThemes,
        MAX_HALLS, matchedIndices);

    cout << "\n==============================================" << endl;
    if (exactCount > 0) {
        cout << "Found " << exactCount << " Exact Matching Hall(s):" << endl;
        cout << "----------------------------------------------" << endl;
        for (int i = 0; i < exactCount; i++) {
            int idx = matchedIndices[i];
            cout << "[Hall ID: " << hallIds[idx] << "] " << hallNames[idx] << endl;
            cout << "   Sector   : " << hallAreas[idx] << " | Capacity: " << hallCapacities[idx] << " guests" << endl;
            cout << "   Occasion : " << hallOccasions[idx] << " | Theme: " << hallThemes[idx] << endl;
            cout << "   Base Rent: Rs. " << hallBasePrices[idx] << endl;
            cout << "----------------------------------------------" << endl;
        }
    }
    else {
        cout << "No exact match found for all 5 filters." << endl;
        cout << "Showing Top Closest Halls (Ranked by Similarity):" << endl;
        cout << "----------------------------------------------" << endl;

        rankFallbackHalls(desiredArea, guestCount, maxBudget, desiredOccasion,
            hallAreas, hallCapacities, hallBasePrices, hallOccasions,
            MAX_HALLS, rankedIndices, matchScores);

        int topCount = 3;
        if (MAX_HALLS < 3) {
            topCount = MAX_HALLS;
        }

        for (int i = 0; i < topCount; i++) {
            int idx = rankedIndices[i];
            cout << i + 1 << ". [Hall ID: " << hallIds[idx] << "] " << hallNames[idx]
                << " (Match Score: " << matchScores[i] << "/5)" << endl;
            cout << "   Sector   : " << hallAreas[idx] << " | Capacity: " << hallCapacities[idx] << " guests" << endl;
            cout << "   Occasion : " << hallOccasions[idx] << " | Theme: " << hallThemes[idx] << endl;
            cout << "   Base Rent: Rs. " << hallBasePrices[idx] << endl;
            cout << "----------------------------------------------" << endl;
        }
    }

    return 0;
}

// ==================== PARTNER 1 (HUSSAIN) DEFINITIONS ====================

void initHalls(int hallIds[], string hallNames[], string hallAreas[], string hallThemes[], string hallOccasions[], int hallCapacities[], double hallBasePrices[]) {
    // --- Sector F-7 (2 Halls) ---
    hallIds[0] = 1;
    hallNames[0] = "Grand Ballroom";
    hallAreas[0] = "F-7";
    hallThemes[0] = "Royal";
    hallOccasions[0] = "Wedding";
    hallCapacities[0] = 300;
    hallBasePrices[0] = 150000.0;

    hallIds[1] = 2;
    hallNames[1] = "Margalla Vista Marquee";
    hallAreas[1] = "F-7";
    hallThemes[1] = "Floral";
    hallOccasions[1] = "Wedding";
    hallCapacities[1] = 600;
    hallBasePrices[1] = 280000.0;

    // --- Sector E-11 (2 Halls) ---
    hallIds[2] = 3;
    hallNames[2] = "Royal Palace";
    hallAreas[2] = "E-11";
    hallThemes[2] = "Floral";
    hallOccasions[2] = "Birthday";
    hallCapacities[2] = 500;
    hallBasePrices[2] = 250000.0;

    hallIds[3] = 4;
    hallNames[3] = "Crystal Banquet Hall";
    hallAreas[3] = "E-11";
    hallThemes[3] = "Modern";
    hallOccasions[3] = "Wedding";
    hallCapacities[3] = 400;
    hallBasePrices[3] = 200000.0;

    // --- Sector G-11 (2 Halls) ---
    hallIds[4] = 5;
    hallNames[4] = "Imperial Arena";
    hallAreas[4] = "G-11";
    hallThemes[4] = "Modern";
    hallOccasions[4] = "Expo";
    hallCapacities[4] = 800;
    hallBasePrices[4] = 400000.0;

    hallIds[5] = 6;
    hallNames[5] = "Pearl Marquee";
    hallAreas[5] = "G-11";
    hallThemes[5] = "Royal";
    hallOccasions[5] = "Birthday";
    hallCapacities[5] = 250;
    hallBasePrices[5] = 130000.0;

    // --- Sector F-10 (1 Hall) ---
    hallIds[6] = 7;
    hallNames[6] = "Emerald Gardens";
    hallAreas[6] = "F-10";
    hallThemes[6] = "Floral";
    hallOccasions[6] = "Wedding";
    hallCapacities[6] = 450;
    hallBasePrices[6] = 220000.0;

    // --- Sector I-8 (1 Hall) ---
    hallIds[7] = 8;
    hallNames[7] = "Capital Convention Center";
    hallAreas[7] = "I-8";
    hallThemes[7] = "Modern";
    hallOccasions[7] = "Expo";
    hallCapacities[7] = 1000;
    hallBasePrices[7] = 500000.0;

    // --- Sector F-8 (1 Hall) ---
    hallIds[8] = 9;
    hallNames[8] = "Crown Heritage Hall";
    hallAreas[8] = "F-8";
    hallThemes[8] = "Royal";
    hallOccasions[8] = "Wedding";
    hallCapacities[8] = 350;
    hallBasePrices[8] = 180000.0;

    // --- Sector G-9 (1 Hall) ---
    hallIds[9] = 10;
    hallNames[9] = "Starlight Event Studio";
    hallAreas[9] = "G-9";
    hallThemes[9] = "Modern";
    hallOccasions[9] = "Birthday";
    hallCapacities[9] = 200;
    hallBasePrices[9] = 110000.0;
}

int searchHalls(string area, int guests, double maxBudget, string occasion, string theme,
    string hallAreas[], int hallCapacities[], double hallBasePrices[], string hallOccasions[], string hallThemes[],
    int totalHalls, int matchedIndices[]) {
    if (totalHalls <= 0) {
        return 0;
    }

    int matchCount = 0;

    for (int i = 0; i < totalHalls; i++) {
        if (hallAreas[i] == area &&
            hallCapacities[i] >= guests &&
            hallBasePrices[i] <= maxBudget &&
            hallOccasions[i] == occasion &&
            hallThemes[i] == theme) {
            matchedIndices[matchCount] = i;
            matchCount++;
        }
    }

    return matchCount;
}

void rankFallbackHalls(string area, int guests, double maxBudget, string occasion,
    string hallAreas[], int hallCapacities[], double hallBasePrices[], string hallOccasions[],
    int totalHalls, int rankedIndices[], int matchScores[]) {
    if (totalHalls <= 0) {
        return;
    }

    for (int i = 0; i < totalHalls; i++) {
        rankedIndices[i] = i;
        matchScores[i] = 0;

        if (hallCapacities[i] >= guests) {
            matchScores[i] += 2;
        }
        if (hallAreas[i] == area) {
            matchScores[i] += 1;
        }
        if (hallOccasions[i] == occasion) {
            matchScores[i] += 1;
        }
        if (hallBasePrices[i] <= maxBudget) {
            matchScores[i] += 1;
        }
    }

    for (int i = 0; i < totalHalls - 1; i++) {
        for (int j = 0; j < totalHalls - i - 1; j++) {
            if (matchScores[j] < matchScores[j + 1]) {
                swap(matchScores[j], matchScores[j + 1]);
                swap(rankedIndices[j], rankedIndices[j + 1]);
            }
        }
    }
}

// ==================== PARTNER 2 DEFINITIONS ====================
// Partner 2 will write their 11 function bodies below this line