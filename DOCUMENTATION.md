# Hall Booking System — Complete Technical Documentation

## 1. Project Overview

The **Hall Booking System** is a full-stack reservation and venue-management application for marriage halls and event venues across Islamabad, Pakistan. It manages **10 halls** across **7 Islamabad sectors** (`F-7`, `E-11`, `G-11`, `F-10`, `I-8`, `F-8`, `G-9`), **3 catering menu packages**, **3 daily time slots** (`Morning`, `Evening`, `Night`), dynamic pricing (weekend surcharges and bulk guest discounts), occasion-based complimentary perks, custom client requests, and persistent file storage (`bookings.txt`).

### Key Capabilities
- **Dual-Mode C++ Backend:**
  1. **HTTP Server Mode (Port `8080`):** Serves the HTML/CSS/JS frontend and exposes a RESTful JSON API using `httplib.h`.
  2. **Console Menu Mode:** Provides a full-featured terminal application for searching halls, checking availability, creating/updating/deleting bookings, sorting reservations, and printing receipts.
- **Interactive Web Frontend (SPA):** A single-page web application (`frontend/index.html`, `frontend/style.css`, `frontend/app.js`) with 6 views and 2 modals, communicating with the C++ backend over `http://localhost:8080/api`.
- **Smart Hall Ranking (Bubble Sort):** Ranks all 10 halls from exact match to closest match using weighted scoring across sector proximity, guest capacity, budget, and slot availability.
- **Persistent File Storage:** Saves and loads all reservations in pipe-delimited format (`bookings.txt`) without requiring an external database.

---

## 2. Project Directory & File Structure

```text
hall-boking-full/
├── Project1/
│   ├── main.cpp                  # Program entry point & mode selection menu
│   ├── httplib.h                 # Single-header C++ HTTP server library (cpp-httplib)
│   ├── bookings.txt              # Persistent pipe-delimited reservation storage (auto-created)
│   ├── Project1.vcxproj          # Visual Studio project configuration
│   ├── Project1.vcxproj.filters  # Visual Studio folder/filter mapping
│   ├── Project1.slnx             # Visual Studio solution file
│   └── src/
│       ├── helpers.cpp           # Date, phone, string, sector, theme, perks, lookup & formatting
│       ├── booking_service.cpp   # Initialization, availability, ranking, pricing & CRUD logic
│       ├── storage.cpp           # File I/O (saveBookings / loadBookings)
│       ├── json_utils.cpp        # Manual JSON parsing & serialization (no external JSON library)
│       ├── http_server.cpp       # HTTP routes, CORS headers, and static frontend mounting
│       └── console_ui.cpp        # Terminal UI prompts, tables, receipts & interactive menu
└── frontend/
    ├── index.html                # Single-page HTML structure (6 views + 2 modals)
    ├── style.css                 # Responsive styling, cards, tables, badges & print styles
    └── app.js                    # Client-side state, API calls, DOM rendering & event handlers
```

---

## 3. System Architecture & Data Flow

1. **Startup (`main.cpp`):**
   - Initializes the 10 hardcoded `Hall` records (`initHalls`) and 3 `MenuPackage` records (`initMenus`).
   - Calls `loadBookings` to read `bookings.txt`. If `bookings.txt` does not exist yet, it calls `seedDefaultBookings` (seeding 2 default bookings `#101` and `#102`) and writes `bookings.txt`.
   - Prompts the user to choose **HTTP Server (`1`)** or **Console Mode (`2`)**.
2. **HTTP Request Lifecycle (Web Mode):**
   - The browser loads `index.html`, `style.css`, and `app.js` from `http://localhost:8080/`.
   - `app.js` uses `fetch()` (wrapped in `apiFetch()`) to send HTTP requests (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`) to `http://localhost:8080/api/...`.
   - `http_server.cpp` receives the request, extracts query/path/JSON parameters using `json_utils.cpp` and `helpers.cpp`, runs business logic in `booking_service.cpp`, persists changes via `storage.cpp`, and returns a JSON response.
   - `app.js` parses the JSON response and updates the HTML DOM dynamically without reloading the page.

---

## 4. Backend Data Models & Constants

### Global Constants
| Constant | Value | Purpose |
| :--- | :--- | :--- |
| `MAX_HALLS` | `10` | Fixed size of the `halls[]` array |
| `MAX_MENUS` | `3` | Fixed size of the `menus[]` array |
| `MAX_SLOTS` | `3` | Number of time slots per day (`1` = Morning, `2` = Evening, `3` = Night) |
| `MAX_MENU_ITEMS` | `6` | Number of dish items stored per `MenuPackage` |
| `MAX_BOOKINGS` | `100` | Maximum capacity of the `bookings[]` array |
| `MIN_YEAR` / `MAX_YEAR` | `2026` / `2035` | Allowed event booking year range |
| `WEEKEND_SURCHARGE` | `25000.0` | Flat surcharge (Rs.) added for Saturday/Sunday bookings |
| `BULK_DISCOUNT_RATE` | `0.10` | 10% discount when `guests > 80%` of the hall's capacity |
| `SERVER_PORT` | `8080` | TCP port for the HTTP server |
| `BOOKINGS_FILE` | `"bookings.txt"` | Filename used for persistent storage |

### `struct Hall`
Represents a marriage hall/venue:
- `int id`: Unique hall ID (`1`–`10`).
- `string name`: Venue name (e.g., `"Grand Ballroom"`).
- `string area`: Islamabad sector (e.g., `"F-7"`).
- `string hours`: Operating hours string (`"09:00 AM - 01:00 AM"`).
- `int capacity`: Maximum guest capacity.
- `double basePrice`: Base hall rental price in Rs.

### `struct MenuPackage`
Represents a catering package:
- `int id`: Menu ID (`1`–`3`).
- `string shortName`: Short label (`"Menu 1"`, `"Menu 2"`, `"Menu 3"`).
- `string name`: Full descriptive name.
- `double pricePerGuest`: Cost per guest in Rs.
- `string items[MAX_MENU_ITEMS]`: Array of 6 menu items.

### `struct Booking`
Represents a complete reservation:
- `int id`: Unique booking ID (starts at `101`).
- `string clientName`, `string clientPhone`: Client contact details.
- `int hallId`, `string hallName`: Selected hall reference and cached name.
- `int day`, `int month`, `int year`: Event date.
- `int slot`: Time slot (`1` = Morning, `2` = Evening, `3` = Night).
- `string theme`: `"Modern"`, `"Floral"`, or `"Royal"`.
- `string occasion`: `"Wedding"`, `"Birthday"`, `"Event"`, or custom text.
- `string perks`: Complimentary perks automatically assigned from `occasion`.
- `string customRequest`: Optional special request from the client.
- `string customRequestStatus`: `"None"`, `"Pending"`, `"Accepted"`, or `"Declined"`.
- `int guests`, `int menuId`: Guest count and selected catering menu ID.
- `double hallBasePrice`, `cateringCost`, `weekendSurcharge`, `discountAmount`, `totalCost`: Calculated billing breakdown.

---

## 5. Backend Function Reference (File by File)

### 5.1 `Project1/main.cpp`
- **`main()`**: Entry point of the application. Sets `cout << fixed` for clean numeric output, allocates `halls[MAX_HALLS]`, `menus[MAX_MENUS]`, and `bookings[MAX_BOOKINGS]`, calls `initHalls`, `initMenus`, and `loadBookings`, then runs a `do-while` loop letting the user start the HTTP server (`1`), launch Console Menu Mode (`2`), or save and exit (`0`).

### 5.2 `Project1/src/helpers.cpp`
Provides reusable validation, parsing, formatting, and lookup utilities used across the entire backend.

#### Date & Phone Validation
- **`isLeapYear(int year)`**: Returns `true` if `year` is a leap year. Used by `daysInMonth` to return 29 days for February.
- **`daysInMonth(int month, int year)`**: Returns the number of days in `month` (`28`–`31`, or `0` if invalid). Prevents invalid dates like `31/02/2026`.
- **`isValidDate(int day, int month, int year)`**: Validates that `year` is within `MIN_YEAR`–`MAX_YEAR` (`2026`–`2035`), `month` is `1`–`12`, and `day` is `1`–`daysInMonth`.
- **`getDayOfWeek(int day, int month, int year)`**: Uses Sakamoto's algorithm to compute the day of the week (`0` = Sunday through `6` = Saturday).
- **`isWeekendDate(int day, int month, int year)`**: Returns `true` if the date falls on Saturday (`6`) or Sunday (`0`). Used to apply `WEEKEND_SURCHARGE`.
- **`isValidPhone(string phone)`**: Trims input, allows an optional leading `+`, verifies all remaining characters are digits, and checks that digit count is `10`–`15`.

#### String Sanitization & Safe Parsing
- **`sanitizeText(string text)`**: Trims leading/trailing whitespace, replaces `|` with a space (preventing corruption of pipe-delimited `bookings.txt`), and strips `\r`/`\n`.
- **`toLowerString(string text)` / `toUpperString(string text)`**: Case-conversion helpers used for case-insensitive search, sector comparison, and theme/slot parsing.
- **`parseIntSafe(string text, int defaultValue)`**: Safely parses an integer string (handling optional `+`/`-` and overflow protection) without throwing exceptions; returns `defaultValue` on invalid input.
- **`parseDoubleSafe(string text, double defaultValue)`**: Safely parses a decimal number string without throwing exceptions; returns `defaultValue` on invalid input.

#### Sector, Theme, Occasion & Perks
- **`isKnownSector(string sector)`**: Validates Islamabad sector format (`[D-I]-[5..18]`, e.g., `F-7`, `E-11`).
- **`extractSectorNumber(string sector)`**: Extracts the numeric part after `-` (e.g., `7` from `F-7`) for proximity scoring.
- **`computeSectorMatchScore(string hallSector, string desiredSector)`**: Scores sector similarity up to `45` points (`45` exact match, `20` if no filter, `+18` same letter series, `+12`/`+8`/`+4` for numeric difference `<= 1` / `<= 2` / `<= 4`).
- **`isValidTheme(string theme)`**: Checks if `theme` (case-insensitive) is `"modern"`, `"floral"`, or `"royal"`.
- **`normalizeTheme(string theme)`**: Returns canonical `"Modern"`, `"Floral"`, or `"Royal"`.
- **`getOccasionPerks(string occasion)`**: Maps the occasion to complimentary perks (e.g., Birthday -> Dancing Gorilla & Mascot + DJ; Wedding/Nikkah/Walima/Mehndi -> Sound System, Live DJ & Bridal Stage Lighting; Event/Seminar/Expo/Conference -> Management Staff & Presentation Audio; Other -> Standard Sound System).

#### Lookup & Formatting
- **`findHallIndexById(const Hall halls[], int hallId)`**: Linearly searches `halls[]` and returns the array index or `-1`.
- **`findMenuIndexById(const MenuPackage menus[], int menuId)`**: Linearly searches `menus[]` and returns the array index or `-1`.
- **`findBookingIndexById(const Booking bookings[], int totalBookings, int bookingId)`**: Linearly searches `bookings[]` and returns the array index or `-1`.
- **`getSlotName(int slot)`**: Maps `1`/`2`/`3` to `"Morning"`, `"Evening"`, `"Night"`.
- **`getSlotTimeRange(int slot)`**: Maps `1`/`2`/`3` to `"09:00 AM - 02:00 PM"`, `"03:00 PM - 07:00 PM"`, `"08:00 PM - 01:00 AM"`.
- **`formatDate(int day, int month, int year)`**: Formats a date as `DD/MM/YYYY`.
- **`formatMoney(double amount)`**: Formats a number with comma separators and `Rs. ` prefix (e.g., `Rs. 150,000`).

### 5.3 `Project1/src/booking_service.cpp`
Contains all domain data initialization and core business logic.

#### Initialization & Seeding
- **`initHalls(Hall halls[])`**: Populates the 10 Islamabad halls across 7 sectors (`F-7`, `E-11`, `G-11`, `F-10`, `I-8`, `F-8`, `G-9`) with capacities (`200`–`1000`) and base prices (`110,000`–`500,000`).
- **`initMenus(MenuPackage menus[])`**: Populates the 3 catering packages (`Basic Banqueting` Rs. 1,200/guest, `Standard Elegance` Rs. 1,800/guest, `Royal Imperial` Rs. 2,500/guest) with 6 items each.
- **`seedDefaultBookings(...)`**: Seeds 2 default reservations (`#101` and `#102`) when no `bookings.txt` file exists yet, computing their perks and costs automatically.

#### Availability, Search & Ranking
- **`isSlotAvailable(...)`**: Checks whether a specific `(hallId, day, month, year, slot)` is free, optionally ignoring `ignoreBookingId` (used when updating an existing booking).
- **`buildAvailabilityMatrix(...)`**: Populates a `10 x 3` boolean matrix (`bookedSlots`) and `bookingIdMatrix` for a given date, plus counts `availableCount` and `bookedCount`.
- **`rankHallsByFilters(...)`**: Scores all 10 halls across 4 factors (Sector: `45`, Capacity: `25` or `8` partial, Budget: `20` or `6` partial, Slot: `15`) and sorts `rankedIndices[]` using **Bubble Sort** (exact matches first, then highest score, then lowest `basePrice`).

#### Pricing, Validation & CRUD
- **`qualifiesForBulkDiscount(int guests, int hallCapacity)`**: Returns `true` when `guests * 100 > hallCapacity * 80` (strictly greater than 80% capacity).
- **`calculateBookingCost(...)`**: Computes `hallBasePrice`, `cateringCost` (`guests * pricePerGuest`), `weekendSurcharge` (`25,000` on Sat/Sun), `discountAmount` (10% of subtotal if eligible), and `totalCost`.
- **`validateBookingInput(...)`**: Validates all booking fields (non-empty name, valid phone, valid date, valid slot, valid hall, guests within hall capacity, valid menu, valid theme, non-empty occasion, and slot availability) and sets `errorMsg` on failure.
- **`getNextBookingId(...)`**: Scans existing bookings to return `maxId + 1` (starting at `101` if empty).
- **`addBooking(...)`**: Sanitizes inputs, validates via `validateBookingInput`, populates a new `Booking` struct (assigning perks and `"Pending"`/`"None"` request status), calculates costs, appends to `bookings[]`, and calls `saveBookings`.
- **`updateBooking(...)`**: Finds the booking by ID, re-validates (ignoring its own ID for slot conflicts), updates all fields and costs, and saves to `bookings.txt`.
- **`deleteBooking(...)`**: Finds the booking by ID, shifts subsequent elements left in `bookings[]`, decrements `totalBookings`, and saves to `bookings.txt`.
- **`searchAndFilterBookings(...)`**: Filters bookings by `hallFilterId` and case-insensitive substring match across ID, `clientName`, `clientPhone`, and `occasion`, returning matching indices in `resultIndices[]`.
- **`sortBookingIndices(...)`**: Uses **Bubble Sort** to sort `indices[]` by `"cost"`, `"date"`, or `"id"` in `"asc"` or `"desc"` order.

### 5.4 `Project1/src/storage.cpp`
- **`saveBookings(const Booking bookings[], int totalBookings, string filename)`**: Writes all bookings to `bookings.txt`, one record per line with 16 pipe-separated (`|`) fields.
- **`loadBookings(...)`**: Opens `bookings.txt`. If missing, calls `seedDefaultBookings` and `saveBookings`. Otherwise, parses each line into 16 fields, validates date/hall/menu, recalculates costs, and populates `bookings[]`.

### 5.5 `Project1/src/json_utils.cpp`
Handles JSON parsing and serialization without any external JSON library.
- **`escapeJson(string text)`**: Escapes `"`, `\`, `\n`, `\r`, `\t` for safe JSON string output.
- **`findJsonKeyValuePos(string body, string key)`**: Locates `"key":` in a JSON request body and returns the index of its value.
- **`extractJsonString(string body, string key)`**: Extracts a string value (handling escape sequences) for `key` from a JSON body.
- **`extractJsonInt(string body, string key, int defaultValue)`**: Extracts an integer value (quoted or unquoted) for `key` from a JSON body.
- **`hallsToJson(const Hall halls[])`**: Serializes all 10 halls as a JSON array.
- **`hallSearchResultsToJson(...)`**: Runs `rankHallsByFilters` and serializes the search metadata plus ranked hall results (including match labels, scores, discount eligibility, and per-slot availability).
- **`menusToJson(const MenuPackage menus[])`**: Serializes surcharge/discount config and all 3 catering menus.
- **`availabilityToJson(...)`**: Runs `buildAvailabilityMatrix` and serializes the `10 x 3` slot matrix for a given date.
- **`bookingToJson(...)`**: Serializes a single `Booking` with enriched hall, menu, date, and billing fields.
- **`bookingsListToJson(...)`**: Serializes a filtered/sorted list of bookings along with `count`, `totalBookings`, and `totalAmount`.

### 5.6 `Project1/src/http_server.cpp`
- **`setCorsHeaders(httplib::Response& res)`**: Adds `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods`, and `Access-Control-Allow-Headers` so browser requests work seamlessly.
- **`startHttpServer(...)`**: Creates `httplib::Server`, auto-detects and mounts the `frontend/` directory at `/`, registers all `/api/*` routes, prints the server banner, and listens on `0.0.0.0:8080`.

### 5.7 `Project1/src/console_ui.cpp`
Provides the complete terminal-based interface:
- **Input Helpers:** `readIntSafe`, `readDoubleSafe`, `readLineSafe` (prevent `cin` failure loops and enforce bounds).
- **Display Helpers:** `printHallsAndMenus`, `printAvailabilityMatrix`, `printReceipt`, `printBookingsTable`.
- **Interactive Flows:** `consoleSearchHalls`, `consoleAddBooking`, `consoleSearchBookings`, `consoleSortBookings`, `consoleViewReceipt`, `consoleUpdateBooking`, `consoleDeleteBooking`.
- **`runConsoleMenu(...)`**: Displays the 11-option (`0`–`10`) main console menu loop.

---

## 6. HTTP REST API Reference

| Method | Endpoint | Purpose | Key Parameters / Body |
| :--- | :--- | :--- | :--- |
| `OPTIONS` | `.*` | CORS preflight | Returns `204` with CORS headers |
| `GET` | `/api/health` | Health check | Returns `{"status":"ok",...}` |
| `GET` | `/api/halls` | List all 10 halls | None |
| `GET` | `/api/halls/search` | Search & rank halls | Query: `sector`, `guests`, `budget`, `day`, `month`, `year`, `slot` |
| `GET` | `/api/menus` | List 3 menus & rules | None |
| `GET` | `/api/availability` | 10×3 slot matrix for date | Query: `day`, `month`, `year` |
| `POST` | `/api/calculate-cost` | Live billing & validation | JSON: `day`, `month`, `year`, `slot`, `hallId`, `guests`, `menuId`, `clientPhone`, `occasion`, `ignoreBookingId` |
| `GET` | `/api/bookings` | List/search/sort bookings | Query: `search`, `hallId`, `sortBy`, `order` |
| `GET` | `/api/bookings/:id` | Get single booking | Path: `id` |
| `POST` | `/api/bookings` | Create booking | JSON: `clientName`, `clientPhone`, `day`, `month`, `year`, `slot`, `hallId`, `guests`, `menuId`, `theme`, `occasion`, `customRequest` |
| `PUT` | `/api/bookings/:id` | Update booking | Path: `id`; JSON: same as `POST` + `customRequestStatus` |
| `PATCH` | `/api/bookings/:id/request` | Approve/decline request | Path: `id`; JSON: `status` (`Accepted`, `Declined`, `Pending`) |
| `DELETE` | `/api/bookings/:id` | Delete booking | Path: `id` |

---

## 7. Frontend Documentation (`HTML`, `CSS`, `JS`)

### 7.1 How the Frontend Works (JavaScript Concepts Used)
- **Single-Page Application (SPA):** `frontend/index.html` contains all 6 views (`view-home`, `view-search-results`, `view-configure`, `view-checkout`, `view-all-halls`, `view-matrix`). Only one view is visible at a time; JavaScript shows/hides them by toggling the CSS class `"hidden"`.
- **`appState` Object:** A central JavaScript object at the top of `app.js` that remembers the loaded `halls`, `menus`, active `currentView`, `sortBy`/`sortOrder`, and the user's current booking wizard selections (`selectedHallId`, `day`, `month`, `year`, `slot`, `guests`, `theme`, `occasionMode`, `customOccasion`, `customRequest`, `menuId`).
- **`async` / `await` and `fetch()`:** Browser network requests take time. Marking a function `async` lets it use `await apiFetch(...)` to wait for the C++ server's JSON response without freezing the browser UI.
- **DOM Manipulation (`document.getElementById`, `.innerHTML`, `.classList`):** JavaScript reads form inputs and updates HTML elements dynamically.
- **Event Listeners (`addEventListener`):** Attached inside `bindEvents()` when the page loads (`DOMContentLoaded`), connecting button clicks, dropdown changes, and form submissions to their handler functions.

### 7.2 `frontend/index.html` Views & Modals
- **Top Navbar & Offline Banner:** Navigation buttons (`Home`, `All 10 Halls`, `Availability Matrix`, `Your Reservations`, `Find a Hall`) and `#connection-banner` (shown if the C++ server is unreachable).
- **View 1 (`#view-home`):** Hero search form (`sector`, `guests`, `budget`, `day`/`month`/`year`, `slot`, sector chips), Featured 3 Halls grid, and **Your Reservations** table with search, hall filter, date/cost sorting, and summary footer.
- **View 2 (`#view-search-results`):** Displays the Bubble-Sorted hall search results with match badges, discount eligibility, and clickable open slot chips.
- **View 3 (`#view-configure`):** Booking customization step—date/slot picker, guest count with live 80% discount hint, theme selector, occasion selector with live perks preview, custom request box, and catering menu cards.
- **View 4 (`#view-checkout`):** Client name/phone inputs, event summary, live billing breakdown card, and validation checklist (`Valid Phone`, `Within Capacity`, `Slot Available`).
- **View 5 (`#view-all-halls`):** Grid displaying all 10 halls.
- **View 6 (`#view-matrix`):** Date selector, quick-date buttons, weekend surcharge badge, and `10 × 3` availability table.
- **Modals:** `#receipt-modal` (printable receipt + Accept/Decline custom request buttons) and `#edit-modal` (full reservation editor).

### 7.3 `frontend/app.js` Function Reference

#### Helpers & Navigation
- **`formatRs(amount)`**: Formats numbers into Pakistani Rupee format (`Rs. 150,000`).
- **`escapeHtml(str)`**: Escapes `&`, `<`, `>`, `"` before inserting user text into `.innerHTML` to prevent XSS/broken HTML.
- **`daysInMonth(month, year)`**: Returns the number of days in a month using JavaScript's `Date` object.
- **`getOccasionValue()`**: Returns either `appState.customOccasion.trim()` (when `"Other"` is selected) or `appState.occasionMode`.
- **`getPerksPreview(occasion)`**: Mirrors the backend's `getOccasionPerks()` so the UI shows perks immediately while typing.
- **`showToast(message, type)`**: Displays a temporary 4-second notification banner (`success` or `error`).
- **`setBackendOffline(isOffline)`**: Shows or hides `#connection-banner`.
- **`apiFetch(path, options)`**: Central `fetch()` wrapper that sends JSON requests to `API_BASE`, handles HTTP errors, and toggles the offline banner on network failure.
- **`switchView(viewId)`**: Hides all views except `viewId`, updates active navbar button styles, and scrolls smoothly to the top.
- **`populateDateSelects(dayId, monthId, yearId, initialDay, initialMonth, initialYear)`**: Fills month (`1`–`12`), year (`2026`–`2035`), and day dropdowns.
- **`syncDaysDropdown(dayId, monthId, yearId, preferredDay)`**: Rebuilds the day `<select>` options whenever month or year changes (e.g., 28/29 days in February).

#### Catalog & Hall Rendering
- **`loadCatalog()`**: Fetches `/halls` and `/menus` in parallel (`Promise.all`), stores them in `appState`, renders featured/all halls, and populates filter/edit dropdowns.
- **`renderHallCards(containerId, hallsList)`**: Generates HTML cards for a list of halls and attaches click listeners to open `openConfigureView`.
- **`populateHallAndMenuSelects()`**: Populates `#bookings-hall-filter`, `#edit-hall`, and `#edit-menu` dropdowns.

#### Search, Configure & Checkout Flow
- **`handleSearchHalls(e)`**: Prevents default form reload, validates sector/guests/budget on the client, calls `GET /api/halls/search`, and switches to `view-search-results`.
- **`renderSearchResults(data, requestedSlot)`**: Renders the Bubble-Sorted hall cards with match badges, slot chips, and click handlers that jump directly into `openConfigureView`.
- **`openConfigureView(hallId, opts)`**: Sets `appState` for the chosen hall/date/slot/guests, renders the hall banner, menus, perks, and live slots, then opens `view-configure`.
- **`refreshConfigureSlots()`**: Fetches `GET /api/availability` for the selected configure date and renders available/booked slot buttons.
- **`updateConfigureDiscountHint()`**: Live-updates the guest count hint (red if invalid, green if `> 80%` capacity).
- **`updateOccasionPerksBox()`**: Updates `#cfg-perks-text` whenever occasion changes.
- **`renderConfigureMenus()`**: Renders the 3 catering menu cards and handles selection clicks.
- **`handleConfigureSubmit(e)`**: Validates guest count and custom occasion, calls `renderCheckoutSummaryAndBilling()`, and switches to `view-checkout`.
- **`renderCheckoutSummaryAndBilling()`**: Renders the event summary and calls `POST /api/calculate-cost` to display live billing totals and validation checkmarks.
- **`setCheckItem(id, valid)`**: Toggles `.valid` / `.invalid` CSS classes and `✓` / `✕` icons on checkout validation items.
- **`handleFinalBooking(e)`**: Sends `POST /api/bookings`, shows a confirmation toast, refreshes bookings and the availability matrix, returns to home, and opens the receipt modal.

#### Reservations, Matrix & Modals
- **`loadBookings()`**: Fetches `GET /api/bookings` with search, hall filter, and sort parameters, renders `#bookings-body`, updates footer totals, and binds View/Edit/Delete button handlers.
- **`loadAvailabilityMatrix()`**: Fetches `GET /api/availability` for the selected matrix date, updates the weekend badge and available/booked counts, and renders the `10 × 3` matrix table.
- **`openReceiptModal(bookingId, isConfirmation)`**: Fetches `GET /api/bookings/:id` and displays the formatted receipt modal (including Accept/Decline buttons if a custom request exists).
- **`updateCustomRequestStatus(bookingId, status, isConfirmation)`**: Sends `PATCH /api/bookings/:id/request` and refreshes both the table and modal.
- **`openEditModal(bookingId)`**: Fetches `GET /api/bookings/:id`, pre-fills all `#edit-*` form fields, and opens `#edit-modal`.
- **`handleUpdateBooking(e)`**: Sends `PUT /api/bookings/:id`, closes the modal, and refreshes bookings and the matrix.
- **`handleDeleteBooking(bookingId)`**: Confirms with the user, sends `DELETE /api/bookings/:id`, and refreshes bookings and the matrix.

#### Events & Bootstrap
- **`toggleSort(field)`**: Toggles `asc`/`desc` sort order for `"date"` or `"cost"` and reloads bookings.
- **`bindEvents()`**: Attaches all DOM event listeners for navigation, search, configuration, checkout, matrix, table toolbar, and modals.
- **`bootstrapApp()`**: Loads catalog, bookings, and availability matrix on startup (and on Retry Connection).
- **`DOMContentLoaded` listener**: Initializes all 4 date pickers (`15/11/2026`), calls `bindEvents()`, and runs `bootstrapApp()`.

---

## 8. How to Modify or Extend the Project

Because the backend and frontend are decoupled over HTTP/JSON, most changes only require editing one or two targeted places:

### 8.1 Adding or Modifying a Hall
1. Increase `MAX_HALLS` in **all 7 `.cpp` files** (or update the shared constant block at the top of each `.cpp` file).
2. Add/edit the hall entry in `initHalls()` inside `Project1/src/booking_service.cpp`.
3. *Frontend:* No changes needed—`loadCatalog()` fetches `/api/halls` dynamically.

### 8.2 Adding or Modifying a Catering Menu
1. Update `MAX_MENUS` / `MAX_MENU_ITEMS` in the `.cpp` files if changing the count.
2. Edit `initMenus()` in `Project1/src/booking_service.cpp`.
3. *Frontend:* No changes needed—`loadCatalog()` fetches `/api/menus` dynamically.

### 8.3 Changing Pricing Rules (Weekend Surcharge or Bulk Discount)
1. **Weekend Surcharge:** Change `WEEKEND_SURCHARGE` in the `.cpp` files (currently `25000.0`). Also update the static label text in `frontend/index.html` (`+Rs. 25,000`) and `frontend/app.js` line 705 if you want the UI label to match.
2. **Bulk Discount:** Change `BULK_DISCOUNT_RATE` (currently `0.10`) in the `.cpp` files, or adjust the `80%` threshold in `qualifiesForBulkDiscount()` (`booking_service.cpp`) and `updateConfigureDiscountHint()` (`frontend/app.js`).

### 8.4 Changing Allowed Year Range
1. Update `MIN_YEAR` and `MAX_YEAR` in the `.cpp` files.
2. Update the `for (let y = 2026; y <= 2035; y++)` loop in `populateDateSelects()` inside `frontend/app.js` (and the subtitle text in `frontend/index.html` if desired).

### 8.5 Changing Hall Search Ranking Weights
- Edit `rankHallsByFilters()` in `Project1/src/booking_service.cpp` and `computeSectorMatchScore()` in `Project1/src/helpers.cpp`.
- Current weights: Sector (`45`), Capacity (`25`), Budget (`20`), Slot (`15`).

### 8.6 Adding a New Theme or Occasion Perk
- **Theme:** Update `isValidTheme()` and `normalizeTheme()` in `Project1/src/helpers.cpp`, plus the theme buttons in `frontend/index.html` (`#cfg-theme-selector`) and `<select id="edit-theme">`.
- **Occasion / Perk:** Update `getOccasionPerks()` in `Project1/src/helpers.cpp` and `getPerksPreview()` in `frontend/app.js` so both stay in sync.

### 8.7 Changing Server Port or Storage Filename
- **Port:** Change `SERVER_PORT` in the `.cpp` files and `API_BASE` on line 1 of `frontend/app.js`.
- **Storage File:** Change `BOOKINGS_FILE` in the `.cpp` files (currently `"bookings.txt"`).

### 8.8 Adding a New Field to `Booking`
1. Add the field to `struct Booking` in all `.cpp` files.
2. Update `addBooking()` and `updateBooking()` in `booking_service.cpp`.
3. Update `saveBookings()` and `loadBookings()` in `storage.cpp` (increasing the pipe-delimited `fields[]` array size).
4. Update `bookingToJson()` in `json_utils.cpp` and the `POST`/`PUT` handlers in `http_server.cpp`.
5. Update `frontend/index.html` and `frontend/app.js` (`handleFinalBooking`, `openEditModal`, `handleUpdateBooking`, `openReceiptModal`) to send and display the new field.
