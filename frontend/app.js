const API_BASE = "http://localhost:8080/api";

const MONTHS = [
  { value: 1, label: "01 – January" },
  { value: 2, label: "02 – February" },
  { value: 3, label: "03 – March" },
  { value: 4, label: "04 – April" },
  { value: 5, label: "05 – May" },
  { value: 6, label: "06 – June" },
  { value: 7, label: "07 – July" },
  { value: 8, label: "08 – August" },
  { value: 9, label: "09 – September" },
  { value: 10, label: "10 – October" },
  { value: 11, label: "11 – November" },
  { value: 12, label: "12 – December" }
];

const appState = {
  halls: [],
  menus: [],
  currentView: "view-home",
  previousView: "view-home",
  sortBy: "id",
  sortOrder: "asc",
  // Active wizard selection
  selectedHallId: 1,
  day: 15,
  month: 11,
  year: 2026,
  slot: 1,
  guests: 250,
  theme: "Modern",
  occasionMode: "Wedding",
  customOccasion: "",
  customRequest: "",
  menuId: 2,
  lastSearchResults: null
};

// ==================== HELPERS ====================
function formatRs(amount) {
  return "Rs. " + Number(amount || 0).toLocaleString("en-PK");
}

function escapeHtml(str) {
  return String(str || "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function daysInMonth(month, year) {
  return new Date(year, month, 0).getDate();
}

function getOccasionValue() {
  if (appState.occasionMode === "Other") {
    return appState.customOccasion.trim();
  }
  return appState.occasionMode;
}

function getPerksPreview(occasion) {
  const occ = String(occasion || "").toLowerCase();
  if (occ.includes("birth")) {
    return "Free Dancing Gorilla & Mascot + Sound System & DJ";
  }
  if (occ === "wedding" || occ.includes("nikkah") || occ.includes("walima") || occ.includes("mehndi")) {
    return "Free Sound System & Live DJ + Bridal Stage Lighting";
  }
  if (occ === "event" || occ.includes("seminar") || occ.includes("expo") || occ.includes("conference")) {
    return "Free Dedicated Management Staff + Presentation Audio";
  }
  return "Free Standard Sound System";
}

function showToast(message, type = "success") {
  const toast = document.getElementById("toast");
  toast.textContent = message;
  toast.className = `toast ${type}`;
  clearTimeout(toast._timer);
  toast._timer = setTimeout(() => toast.classList.add("hidden"), 4000);
}

function setBackendOffline(isOffline) {
  document.getElementById("connection-banner").classList.toggle("hidden", !isOffline);
}

async function apiFetch(path, options = {}) {
  try {
    const res = await fetch(`${API_BASE}${path}`, {
      headers: { "Content-Type": "application/json" },
      ...options
    });
    const data = await res.json();
    setBackendOffline(false);
    if (!res.ok) {
      throw new Error(data.error || `Request failed (${res.status})`);
    }
    return data;
  } catch (err) {
    if (err instanceof TypeError) {
      setBackendOffline(true);
      throw new Error("Cannot reach C++ server at http://localhost:8080");
    }
    throw err;
  }
}

function switchView(viewId) {
  const views = ["view-home", "view-search-results", "view-configure", "view-checkout", "view-all-halls", "view-matrix"];
  views.forEach((v) => document.getElementById(v).classList.toggle("hidden", v !== viewId));
  appState.currentView = viewId;

  document.getElementById("nav-home-btn").classList.toggle("active", viewId === "view-home");
  document.getElementById("nav-all-halls-btn").classList.toggle("active", viewId === "view-all-halls");
  document.getElementById("nav-matrix-btn").classList.toggle("active", viewId === "view-matrix");
  document.getElementById("nav-reservations-btn").classList.remove("active");
  window.scrollTo({ top: 0, behavior: "smooth" });
}

function populateDateSelects(dayId, monthId, yearId, initialDay, initialMonth, initialYear) {
  const monthEl = document.getElementById(monthId);
  const yearEl = document.getElementById(yearId);

  monthEl.innerHTML = MONTHS.map((m) => `<option value="${m.value}">${m.label}</option>`).join("");
  let yearsHtml = "";
  for (let y = 2026; y <= 2035; y++) {
    yearsHtml += `<option value="${y}">${y}</option>`;
  }
  yearEl.innerHTML = yearsHtml;

  monthEl.value = String(initialMonth);
  yearEl.value = String(initialYear);
  syncDaysDropdown(dayId, monthId, yearId, initialDay);
}

function syncDaysDropdown(dayId, monthId, yearId, preferredDay = null) {
  const dayEl = document.getElementById(dayId);
  const month = Number(document.getElementById(monthId).value);
  const year = Number(document.getElementById(yearId).value);
  const maxDays = daysInMonth(month, year);
  const currentDay = preferredDay !== null ? preferredDay : Math.min(Number(dayEl.value || 1), maxDays);

  let html = "";
  for (let d = 1; d <= maxDays; d++) {
    html += `<option value="${d}">${d}</option>`;
  }
  dayEl.innerHTML = html;
  dayEl.value = String(Math.min(currentDay, maxDays));
}

// ==================== INITIAL LOAD ====================
async function loadCatalog() {
  const [halls, menuData] = await Promise.all([apiFetch("/halls"), apiFetch("/menus")]);
  appState.halls = halls;
  appState.menus = menuData.menus || [];

  renderHallCards("home-featured-halls", halls.slice(0, 3));
  renderHallCards("all-halls-grid", halls);
  populateHallAndMenuSelects();
}

function renderHallCards(containerId, hallsList) {
  const grid = document.getElementById(containerId);
  grid.innerHTML = hallsList
    .map(
      (h) => `
      <article class="card hall-card">
        <div class="hall-card-head">
          <div class="hall-card-top">
            <span class="pill pill-slate">Hall #${h.id}</span>
            <span class="pill pill-amber">Sector ${escapeHtml(h.area)}</span>
          </div>
          <h3 class="hall-card-title">${escapeHtml(h.name)}</h3>
        </div>
        <div class="hall-meta">
          <div class="hall-meta-item">
            <span class="hall-meta-icon">👥</span>
            <div>
              <div class="hall-meta-label">Maximum Capacity</div>
              <div class="hall-meta-value">${h.capacity} Guests</div>
            </div>
          </div>
          <div class="hall-meta-item">
            <span class="hall-meta-icon">$</span>
            <div>
              <div class="hall-meta-label">Base Price</div>
              <div class="hall-meta-value">${formatRs(h.basePrice)}</div>
            </div>
          </div>
        </div>
        <button type="button" class="btn btn-outline choose-hall-btn" data-hall-id="${h.id}">
          Select &amp; Customize →
        </button>
      </article>
    `
    )
    .join("");

  grid.querySelectorAll(".choose-hall-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      openConfigureView(Number(btn.dataset.hallId), {
        day: Number(document.getElementById("search-day").value),
        month: Number(document.getElementById("search-month").value),
        year: Number(document.getElementById("search-year").value),
        slot: Number(document.getElementById("search-slot").value) || 1,
        guests: Number(document.getElementById("search-guests").value) || 200,
        fromView: appState.currentView
      });
    });
  });
}

function populateHallAndMenuSelects() {
  const filterHall = document.getElementById("bookings-hall-filter");
  filterHall.innerHTML =
    `<option value="0">All Halls</option>` +
    appState.halls.map((h) => `<option value="${h.id}">${escapeHtml(h.name)} (${escapeHtml(h.area)})</option>`).join("");

  document.getElementById("edit-hall").innerHTML = appState.halls
    .map((h) => `<option value="${h.id}">${escapeHtml(h.name)} (${escapeHtml(h.area)}, Max ${h.capacity})</option>`)
    .join("");

  document.getElementById("edit-menu").innerHTML = appState.menus
    .map((m) => `<option value="${m.id}">${escapeHtml(m.name)} (${formatRs(m.pricePerGuest)}/guest)</option>`)
    .join("");
}

// ==================== STEP 1 -> 2: SEARCH HALLS (BUBBLE SORT) ====================
async function handleSearchHalls(e) {
  e.preventDefault();
  const errorBox = document.getElementById("search-error-box");
  errorBox.classList.add("hidden");

  const sector = document.getElementById("search-sector").value.trim().toUpperCase();
  const guestsRaw = document.getElementById("search-guests").value.trim();
  const budgetRaw = document.getElementById("search-budget").value.trim();
  const day = Number(document.getElementById("search-day").value);
  const month = Number(document.getElementById("search-month").value);
  const year = Number(document.getElementById("search-year").value);
  const slot = Number(document.getElementById("search-slot").value);

  // Client-side validation + reprompt before calling backend
  if (!sector) {
    errorBox.textContent = "Please enter an Islamabad sector (e.g., F-7, E-11, G-11, F-10, I-8, F-8, G-9).";
    errorBox.classList.remove("hidden");
    document.getElementById("search-sector").focus();
    return;
  }
  const guests = Number(guestsRaw);
  if (!guestsRaw || isNaN(guests) || guests <= 0 || guests > 1000) {
    errorBox.textContent = "Please enter a valid number of guests between 1 and 1000.";
    errorBox.classList.remove("hidden");
    document.getElementById("search-guests").focus();
    return;
  }
  const budget = Number(budgetRaw);
  if (!budgetRaw || isNaN(budget) || budget <= 0) {
    errorBox.textContent = "Please enter a valid positive maximum budget in Rs.";
    errorBox.classList.remove("hidden");
    document.getElementById("search-budget").focus();
    return;
  }

  try {
    const query = `sector=${encodeURIComponent(sector)}&guests=${guests}&budget=${budget}&day=${day}&month=${month}&year=${year}&slot=${slot}`;
    const data = await apiFetch(`/halls/search?${query}`);
    appState.lastSearchResults = data;
    appState.guests = guests;
    appState.day = day;
    appState.month = month;
    appState.year = year;
    renderSearchResults(data, slot);
    switchView("view-search-results");
  } catch (err) {
    errorBox.textContent = err.message;
    errorBox.classList.remove("hidden");
  }
}

function renderSearchResults(data, requestedSlot) {
  document.getElementById("search-summary-pill").textContent =
    `Sector: ${data.sector} • ${data.guests} Guests • Budget: ${formatRs(data.budget)} • ${data.dateFormatted}`;

  const list = document.getElementById("search-results-list");
  list.innerHTML = data.results
    .map((h) => {
      const badgeClass = h.exactMatch
        ? "pill-green"
        : h.matchLabel === "Sector Match" || h.matchLabel === "Close Match"
        ? "pill-amber"
        : "pill-neutral";

      return `
        <article class="card result-card ${h.exactMatch ? "exact" : ""}">
          <div class="result-main">
            <div class="result-badges">
              <span class="pill pill-slate">Hall #${h.id}</span>
              <span class="pill pill-amber">Sector ${escapeHtml(h.area)}</span>
              <span class="pill ${badgeClass}">${escapeHtml(h.matchLabel)} (${h.matchScore}/100)</span>
              ${h.qualifiesDiscount ? `<span class="pill pill-green">10% Bulk Discount Eligible</span>` : ""}
            </div>
            <h3 class="result-title">${escapeHtml(h.name)}</h3>
            <div class="result-meta">
              Max Capacity: <strong>${h.capacity} Guests</strong> • Base Price: <strong>${formatRs(h.basePrice)}</strong> • Hours: ${escapeHtml(h.hours)}
            </div>
            <div class="result-slots">
              ${h.availableSlots
                .map((s) =>
                  s.available
                    ? `<button type="button" class="slot-chip open pick-result-slot" data-hall-id="${h.id}" data-slot="${s.slot}">
                        ✓ ${escapeHtml(s.slotName)} (${escapeHtml(s.timeRange)})
                       </button>`
                    : `<span class="slot-chip closed">✕ ${escapeHtml(s.slotName)} Booked</span>`
                )
                .join("")}
            </div>
          </div>

          <div>
            <button type="button" class="btn btn-dark pick-result-hall" data-hall-id="${h.id}">
              Choose This Hall →
            </button>
          </div>
        </article>
      `;
    })
    .join("");

  list.querySelectorAll(".pick-result-hall").forEach((btn) => {
    btn.addEventListener("click", () => {
      const hallId = Number(btn.dataset.hallId);
      const hallObj = data.results.find((r) => r.id === hallId);
      let defaultSlot = requestedSlot > 0 ? requestedSlot : 1;
      if (hallObj) {
        const firstOpen = hallObj.availableSlots.find((s) => s.available && (requestedSlot === 0 || s.slot === requestedSlot))
          || hallObj.availableSlots.find((s) => s.available);
        if (firstOpen) defaultSlot = firstOpen.slot;
      }
      openConfigureView(hallId, {
        day: data.day,
        month: data.month,
        year: data.year,
        slot: defaultSlot,
        guests: data.guests,
        fromView: "view-search-results"
      });
    });
  });

  list.querySelectorAll(".pick-result-slot").forEach((btn) => {
    btn.addEventListener("click", () => {
      openConfigureView(Number(btn.dataset.hallId), {
        day: data.day,
        month: data.month,
        year: data.year,
        slot: Number(btn.dataset.slot),
        guests: data.guests,
        fromView: "view-search-results"
      });
    });
  });
}

// ==================== STEP 3: CONFIGURE THEME, OCCASION, PERKS & MENU ====================
async function openConfigureView(hallId, opts) {
  appState.selectedHallId = hallId;
  appState.day = opts.day;
  appState.month = opts.month;
  appState.year = opts.year;
  appState.slot = opts.slot || 1;
  appState.previousView = opts.fromView || "view-home";

  const hall = appState.halls.find((h) => h.id === hallId);
  if (!hall) return;

  appState.guests = Math.min(Math.max(Number(opts.guests || 200), 1), hall.capacity);

  document.getElementById("cfg-hall-banner").innerHTML = `
    <div>
      <span class="pill pill-amber">Sector ${escapeHtml(hall.area)}</span>
      <h2 style="margin-top:6px;">${escapeHtml(hall.name)}</h2>
      <p class="section-subtitle">Maximum Capacity: ${hall.capacity} Guests • Base Rent: ${formatRs(hall.basePrice)}</p>
    </div>
    <div style="text-align:right;">
      <span class="pill pill-green">10% OFF if guests &gt; ${Math.floor(hall.capacity * 0.8)}</span>
    </div>
  `;

  document.getElementById("cfg-month").value = String(appState.month);
  document.getElementById("cfg-year").value = String(appState.year);
  syncDaysDropdown("cfg-day", "cfg-month", "cfg-year", appState.day);

  const guestsEl = document.getElementById("cfg-guests");
  guestsEl.max = String(hall.capacity);
  guestsEl.value = String(appState.guests);
  document.getElementById("cfg-guests-label").textContent = `Number of Guests (Max ${hall.capacity})`;

  updateConfigureDiscountHint();
  renderConfigureMenus();
  updateOccasionPerksBox();
  await refreshConfigureSlots();

  document.getElementById("cfg-error-box").classList.add("hidden");
  switchView("view-configure");
}

async function refreshConfigureSlots() {
  const day = Number(document.getElementById("cfg-day").value);
  const month = Number(document.getElementById("cfg-month").value);
  const year = Number(document.getElementById("cfg-year").value);
  appState.day = day;
  appState.month = month;
  appState.year = year;

  const availData = await apiFetch(`/availability?day=${day}&month=${month}&year=${year}`);
  const hallAvail = availData.halls.find((h) => h.hallId === appState.selectedHallId);
  const container = document.getElementById("cfg-slot-selector");
  if (!hallAvail) return;

  container.innerHTML = hallAvail.slots
    .map((s) => {
      const isSelected = appState.slot === s.slot;
      const cls = !s.available ? "slot-option disabled" : isSelected ? "slot-option active" : "slot-option";
      return `
        <button type="button" class="${cls}" data-slot="${s.slot}" ${!s.available ? "disabled" : ""}>
          <span class="slot-option-name">${escapeHtml(s.slotName)} ${!s.available ? "(Booked)" : ""}</span>
          <span class="slot-option-time">${escapeHtml(s.timeRange)}</span>
        </button>
      `;
    })
    .join("");

  container.querySelectorAll(".slot-option:not(.disabled)").forEach((btn) => {
    btn.addEventListener("click", () => {
      appState.slot = Number(btn.dataset.slot);
      container.querySelectorAll(".slot-option").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
    });
  });
}

function updateConfigureDiscountHint() {
  const hall = appState.halls.find((h) => h.id === appState.selectedHallId);
  const guests = Number(document.getElementById("cfg-guests").value || 0);
  const hint = document.getElementById("cfg-discount-hint");
  if (!hall) return;

  const threshold = Math.floor(hall.capacity * 0.8);
  if (guests > hall.capacity || guests <= 0) {
    hint.textContent = `Enter a valid guest count between 1 and ${hall.capacity}.`;
    hint.style.color = "var(--red)";
  } else if (guests * 100 > hall.capacity * 80) {
    hint.textContent = `✓ 10% Bulk Discount Unlocked! (${guests} > 80% of ${hall.capacity})`;
    hint.style.color = "var(--green)";
  } else {
    hint.textContent = `Tip: Book more than ${threshold} guests (>80%) to get a 10% discount.`;
    hint.style.color = "var(--text-muted)";
  }
}

function updateOccasionPerksBox() {
  const occ = getOccasionValue() || appState.occasionMode;
  document.getElementById("cfg-perks-text").textContent = getPerksPreview(occ);
}

function renderConfigureMenus() {
  const container = document.getElementById("cfg-menu-selector");
  container.innerHTML = appState.menus
    .map(
      (m) => `
      <button type="button" class="menu-choice-card ${m.id === appState.menuId ? "active" : ""}" data-menu-id="${m.id}">
        <div class="menu-choice-top">
          <span class="menu-choice-name">${escapeHtml(m.name)}</span>
          <span class="menu-choice-rate">${formatRs(m.pricePerGuest)} / guest</span>
        </div>
        <div class="menu-choice-items">${m.items.map(escapeHtml).join(" • ")}</div>
      </button>
    `
    )
    .join("");

  container.querySelectorAll(".menu-choice-card").forEach((btn) => {
    btn.addEventListener("click", () => {
      appState.menuId = Number(btn.dataset.menuId);
      container.querySelectorAll(".menu-choice-card").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
    });
  });
}

async function handleConfigureSubmit(e) {
  e.preventDefault();
  const errorBox = document.getElementById("cfg-error-box");
  errorBox.classList.add("hidden");

  const hall = appState.halls.find((h) => h.id === appState.selectedHallId);
  const guests = Number(document.getElementById("cfg-guests").value);
  if (!hall || isNaN(guests) || guests <= 0 || guests > hall.capacity) {
    errorBox.textContent = `Please enter a valid guest count between 1 and ${hall ? hall.capacity : 1000}.`;
    errorBox.classList.remove("hidden");
    return;
  }

  if (appState.occasionMode === "Other" && !appState.customOccasion.trim()) {
    errorBox.textContent = "Please type your custom occasion name.";
    errorBox.classList.remove("hidden");
    document.getElementById("cfg-custom-occasion").focus();
    return;
  }

  appState.guests = guests;
  appState.customRequest = document.getElementById("cfg-custom-request").value.trim();

  await renderCheckoutSummaryAndBilling();
  switchView("view-checkout");
}

// ==================== STEP 4: CHECKOUT & BILLING ====================
async function renderCheckoutSummaryAndBilling() {
  const hall = appState.halls.find((h) => h.id === appState.selectedHallId);
  const menu = appState.menus.find((m) => m.id === appState.menuId);
  const occ = getOccasionValue();
  const phone = document.getElementById("chk-client-phone").value.trim();

  const slotLabels = { 1: "Morning (09:00 AM – 02:00 PM)", 2: "Evening (03:00 PM – 07:00 PM)", 3: "Night (08:00 PM – 01:00 AM)" };
  const dateStr = `${String(appState.day).padStart(2, "0")}/${String(appState.month).padStart(2, "0")}/${appState.year}`;

  document.getElementById("checkout-summary-box").innerHTML = `
    <div class="summary-item"><div class="summary-item-label">Hall &amp; Sector</div><div class="summary-item-val">${escapeHtml(hall.name)} (${escapeHtml(hall.area)})</div></div>
    <div class="summary-item"><div class="summary-item-label">Date &amp; Slot</div><div class="summary-item-val">${dateStr} • ${slotLabels[appState.slot]}</div></div>
    <div class="summary-item"><div class="summary-item-label">Theme &amp; Occasion</div><div class="summary-item-val">${escapeHtml(appState.theme)} • ${escapeHtml(occ)}</div></div>
    <div class="summary-item"><div class="summary-item-label">Guests &amp; Menu</div><div class="summary-item-val">${appState.guests} Guests • ${escapeHtml(menu.shortName)}</div></div>
    <div class="summary-item" style="grid-column: 1 / -1;"><div class="summary-item-label">Complimentary Special Perks</div><div class="summary-item-val">${escapeHtml(getPerksPreview(occ))}</div></div>
    ${appState.customRequest ? `<div class="summary-item" style="grid-column: 1 / -1;"><div class="summary-item-label">Separate Request (Pending Approval)</div><div class="summary-item-val">${escapeHtml(appState.customRequest)}</div></div>` : ""}
  `;

  try {
    const res = await apiFetch("/calculate-cost", {
      method: "POST",
      body: JSON.stringify({
        clientPhone: phone,
        day: appState.day,
        month: appState.month,
        year: appState.year,
        slot: appState.slot,
        hallId: appState.selectedHallId,
        guests: appState.guests,
        menuId: appState.menuId,
        occasion: occ
      })
    });

    document.getElementById("bill-hall-name").textContent = `${hall.name} (${hall.area})`;
    document.getElementById("bill-hall-price").textContent = formatRs(res.hallBasePrice);
    document.getElementById("bill-catering-detail").textContent = `${appState.guests} Guests × ${formatRs(menu.pricePerGuest)}`;
    document.getElementById("bill-catering-price").textContent = formatRs(res.cateringSubtotal);
    document.getElementById("bill-weekend-detail").textContent = res.isWeekend ? "Saturday / Sunday (+Rs. 25,000)" : "Weekday (No Surcharge)";
    document.getElementById("bill-weekend-price").textContent = res.isWeekend ? `+${formatRs(res.weekendSurcharge)}` : "Rs. 0";

    const discRow = document.getElementById("bill-discount-row");
    if (res.discountApplied && res.discountAmount > 0) {
      discRow.classList.remove("hidden");
      document.getElementById("bill-discount-price").textContent = `-${formatRs(res.discountAmount)}`;
    } else {
      discRow.classList.add("hidden");
    }

    document.getElementById("bill-total-price").textContent = formatRs(res.totalCost);

    setCheckItem("val-phone", res.validPhone);
    setCheckItem("val-capacity", res.withinCapacity);
    document.getElementById("val-capacity-text").textContent = `Within Capacity (${hall.capacity} max)`;
    setCheckItem("val-slot", res.slotAvailable);
    document.getElementById("val-slot-text").textContent = res.slotAvailable ? "Slot Available" : "Slot Already Booked";
  } catch (err) {
    // ignore transient typing errors
  }
}

function setCheckItem(id, valid) {
  const el = document.getElementById(id);
  el.classList.toggle("valid", valid);
  el.classList.toggle("invalid", !valid);
  el.querySelector(".chk-icon").textContent = valid ? "✓" : "✕";
}

async function handleFinalBooking(e) {
  e.preventDefault();
  const errorBox = document.getElementById("checkout-error-box");
  errorBox.classList.add("hidden");

  const payload = {
    clientName: document.getElementById("chk-client-name").value.trim(),
    clientPhone: document.getElementById("chk-client-phone").value.trim(),
    day: appState.day,
    month: appState.month,
    year: appState.year,
    slot: appState.slot,
    hallId: appState.selectedHallId,
    guests: appState.guests,
    menuId: appState.menuId,
    theme: appState.theme,
    occasion: getOccasionValue(),
    customRequest: appState.customRequest
  };

  try {
    const created = await apiFetch("/bookings", {
      method: "POST",
      body: JSON.stringify(payload)
    });

    showToast(`Confirmation: Hall booked (#${created.id}) and saved to bookings.txt!`, "success");
    document.getElementById("chk-client-name").value = "";
    document.getElementById("cfg-custom-request").value = "";

    await Promise.all([loadBookings(), loadAvailabilityMatrix()]);
    switchView("view-home");
    openReceiptModal(created.id, true);
  } catch (err) {
    errorBox.textContent = err.message;
    errorBox.classList.remove("hidden");
  }
}

// ==================== YOUR RESERVATIONS ====================
async function loadBookings() {
  const search = encodeURIComponent(document.getElementById("bookings-search").value.trim());
  const hallId = document.getElementById("bookings-hall-filter").value || "0";

  const data = await apiFetch(`/bookings?search=${search}&hallId=${hallId}&sortBy=${appState.sortBy}&order=${appState.sortOrder}`);
  const tbody = document.getElementById("bookings-body");

  if (!data.bookings || data.bookings.length === 0) {
    tbody.innerHTML = `<tr><td colspan="10" style="text-align:center; padding: 30px; color:#64748b;">No reservations found.</td></tr>`;
  } else {
    tbody.innerHTML = data.bookings
      .map((b) => {
        const slotCls = b.slotName.toLowerCase();
        const reqBadge = b.customRequest
          ? `<div class="sub-note" style="margin-top:4px;">Req: ${escapeHtml(b.customRequest)} <span class="pill ${b.customRequestStatus === "Accepted" ? "pill-green" : b.customRequestStatus === "Declined" ? "pill-red" : "pill-amber"}">${escapeHtml(b.customRequestStatus)}</span></div>`
          : "";

        return `
          <tr>
            <td><span class="booking-id-badge">#${b.id}</span></td>
            <td>
              <div class="client-name">${escapeHtml(b.clientName)}</div>
              <div class="client-phone">${escapeHtml(b.clientPhone)}</div>
            </td>
            <td>
              <div class="event-date">${escapeHtml(b.dateFormatted)}</div>
              ${b.isWeekend ? `<span class="weekend-tag">Weekend</span>` : ""}
            </td>
            <td><span class="slot-badge ${slotCls}">${escapeHtml(b.slotName)}</span></td>
            <td>
              <div>${escapeHtml(b.hallName)}</div>
              <div class="sub-note">Sector ${escapeHtml(b.hallArea)}</div>
            </td>
            <td>
              <div><strong>${escapeHtml(b.occasion)}</strong> • ${escapeHtml(b.theme)}</div>
              <div class="sub-note">${escapeHtml(b.perks)}</div>
              ${reqBadge}
            </td>
            <td>${b.guests}</td>
            <td>${b.discountAmount > 0 ? `<span class="pill pill-green">-${formatRs(b.discountAmount)}</span>` : "—"}</td>
            <td><span class="booking-total">${formatRs(b.totalCost)}</span></td>
            <td>
              <div class="actions-cell">
                <button type="button" class="action-btn view-btn" data-id="${b.id}" title="View Receipt">📄</button>
                <button type="button" class="action-btn edit-btn" data-id="${b.id}" title="Edit / Approve Request">✏️</button>
                <button type="button" class="action-btn delete delete-btn" data-id="${b.id}" title="Delete">🗑️</button>
              </div>
            </td>
          </tr>
        `;
      })
      .join("");
  }

  document.getElementById("footer-bookings-count").textContent = data.count;
  document.getElementById("footer-total-amount").textContent = formatRs(data.totalAmount);

  tbody.querySelectorAll(".view-btn").forEach((b) => b.addEventListener("click", () => openReceiptModal(Number(b.dataset.id), false)));
  tbody.querySelectorAll(".edit-btn").forEach((b) => b.addEventListener("click", () => openEditModal(Number(b.dataset.id))));
  tbody.querySelectorAll(".delete-btn").forEach((b) => b.addEventListener("click", () => handleDeleteBooking(Number(b.dataset.id))));
}

// ==================== MATRIX ====================
async function loadAvailabilityMatrix() {
  const day = Number(document.getElementById("matrix-day").value);
  const month = Number(document.getElementById("matrix-month").value);
  const year = Number(document.getElementById("matrix-year").value);

  const data = await apiFetch(`/availability?day=${day}&month=${month}&year=${year}`);
  document.querySelectorAll(".matrix-qdate").forEach((btn) => {
    btn.classList.toggle("active", Number(btn.dataset.day) === day && Number(btn.dataset.month) === month && Number(btn.dataset.year) === year);
  });

  const badge = document.getElementById("matrix-weekend-badge");
  badge.className = data.isWeekend ? "pill pill-amber" : "pill pill-neutral";
  badge.textContent = data.isWeekend ? "• Weekend (+Rs. 25,000 Surcharge)" : "Weekday (No Surcharge)";

  document.getElementById("matrix-available-count").textContent = `${data.availableCount} Available`;
  document.getElementById("matrix-booked-count").textContent = `${data.bookedCount} Booked`;

  const tbody = document.getElementById("matrix-body");
  tbody.innerHTML = data.halls
    .map(
      (hall) => `
      <tr>
        <td>
          <div class="matrix-hall-name">${escapeHtml(hall.hallName)} <span class="pill pill-slate">${escapeHtml(hall.area)}</span></div>
          <div class="matrix-hall-sub">Capacity: ${hall.capacity} • ${formatRs(hall.basePrice)}</div>
        </td>
        ${hall.slots
          .map((s) =>
            s.available
              ? `<td><div class="slot-cell-box available"><span>✓ Available</span><button type="button" class="slot-select-btn" data-hall-id="${hall.hallId}" data-slot="${s.slot}">Book →</button></div></td>`
              : `<td><div class="slot-cell-box booked"><span>✕ Booked (#${s.bookingId})</span></div></td>`
          )
          .join("")}
      </tr>
    `
    )
    .join("");

  tbody.querySelectorAll(".slot-select-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      openConfigureView(Number(btn.dataset.hallId), {
        day: data.day,
        month: data.month,
        year: data.year,
        slot: Number(btn.dataset.slot),
        guests: 200,
        fromView: "view-matrix"
      });
    });
  });
}

// ==================== RECEIPT, EDIT & DELETE ====================
async function openReceiptModal(bookingId, isConfirmation) {
  try {
    const b = await apiFetch(`/bookings/${bookingId}`);
    document.getElementById("receipt-modal-title").textContent = isConfirmation
      ? `Booking Confirmed! Receipt #${b.id}`
      : `Booking Receipt #${b.id}`;

    const reqSection = b.customRequest
      ? `
        <div class="receipt-row">
          <span>Custom Request (${escapeHtml(b.customRequestStatus)})</span>
          <strong>${escapeHtml(b.customRequest)}</strong>
        </div>
        <div style="display:flex; gap:8px; margin:8px 0;">
          <button type="button" class="btn-outline-sm btn-accept" id="receipt-accept-req">✓ Accept Request</button>
          <button type="button" class="btn-outline-sm btn-decline" id="receipt-decline-req">✕ Decline Request</button>
        </div>
      `
      : "";

    document.getElementById("receipt-modal-body").innerHTML = `
      ${isConfirmation ? `<div class="perks-banner" style="margin-top:0;margin-bottom:14px;background:var(--green-bg);border-color:var(--green-border);color:var(--green);"><strong>✓ Hall Booked &amp; Saved to bookings.txt!</strong></div>` : ""}
      <div class="receipt-row"><span>Client Name</span><strong>${escapeHtml(b.clientName)}</strong></div>
      <div class="receipt-row"><span>Phone Number</span><strong>${escapeHtml(b.clientPhone)}</strong></div>
      <div class="receipt-row"><span>Hall &amp; Sector</span><strong>${escapeHtml(b.hallName)} (${escapeHtml(b.hallArea)})</strong></div>
      <div class="receipt-row"><span>Date &amp; Slot</span><strong>${escapeHtml(b.dateFormatted)} • ${escapeHtml(b.slotName)}</strong></div>
      <div class="receipt-row"><span>Theme &amp; Occasion</span><strong>${escapeHtml(b.theme)} • ${escapeHtml(b.occasion)}</strong></div>
      <div class="receipt-row"><span>Free Special Perks</span><strong>${escapeHtml(b.perks)}</strong></div>
      ${reqSection}
      <div class="receipt-row"><span>Guests &amp; Menu</span><strong>${b.guests} Guests • ${escapeHtml(b.menuName)}</strong></div>
      <div class="receipt-row"><span>Hall Base Price</span><strong>${formatRs(b.hallBasePrice)}</strong></div>
      <div class="receipt-row"><span>Catering (${b.guests} × ${formatRs(b.pricePerGuest)})</span><strong>${formatRs(b.cateringSubtotal)}</strong></div>
      <div class="receipt-row"><span>Weekend Surcharge</span><strong>${formatRs(b.weekendSurcharge)}</strong></div>
      <div class="receipt-row"><span>10% Bulk Discount</span><strong style="color:var(--green);">-${formatRs(b.discountAmount)}</strong></div>
      <div class="receipt-row total"><span>Total Bill</span><span>${formatRs(b.totalCost)}</span></div>
    `;

    const acceptBtn = document.getElementById("receipt-accept-req");
    const declineBtn = document.getElementById("receipt-decline-req");
    if (acceptBtn) {
      acceptBtn.addEventListener("click", () => updateCustomRequestStatus(b.id, "Accepted", isConfirmation));
    }
    if (declineBtn) {
      declineBtn.addEventListener("click", () => updateCustomRequestStatus(b.id, "Declined", isConfirmation));
    }

    document.getElementById("receipt-modal").classList.remove("hidden");
  } catch (err) {
    showToast(err.message, "error");
  }
}

async function updateCustomRequestStatus(bookingId, status, isConfirmation) {
  try {
    await apiFetch(`/bookings/${bookingId}/request`, {
      method: "PATCH",
      body: JSON.stringify({ status })
    });
    showToast(`Custom request #${bookingId} marked as ${status}.`, "success");
    await loadBookings();
    await openReceiptModal(bookingId, isConfirmation);
  } catch (err) {
    showToast(err.message, "error");
  }
}

async function openEditModal(bookingId) {
  try {
    const b = await apiFetch(`/bookings/${bookingId}`);
    document.getElementById("edit-error-box").classList.add("hidden");
    document.getElementById("edit-modal-title").textContent = `Edit Reservation #${b.id}`;
    document.getElementById("edit-id").value = b.id;
    document.getElementById("edit-name").value = b.clientName;
    document.getElementById("edit-phone").value = b.clientPhone;
    document.getElementById("edit-month").value = String(b.month);
    document.getElementById("edit-year").value = String(b.year);
    syncDaysDropdown("edit-day", "edit-month", "edit-year", b.day);
    document.getElementById("edit-slot").value = String(b.slot);
    document.getElementById("edit-hall").value = String(b.hallId);
    document.getElementById("edit-guests").value = b.guests;
    document.getElementById("edit-menu").value = String(b.menuId);
    document.getElementById("edit-theme").value = b.theme || "Modern";
    document.getElementById("edit-occasion").value = b.occasion || "Wedding";
    document.getElementById("edit-request").value = b.customRequest || "";
    document.getElementById("edit-request-status").value =
      b.customRequestStatus && b.customRequestStatus !== "None" ? b.customRequestStatus : "Pending";

    document.getElementById("edit-modal").classList.remove("hidden");
  } catch (err) {
    showToast(err.message, "error");
  }
}

async function handleUpdateBooking(e) {
  e.preventDefault();
  const errorBox = document.getElementById("edit-error-box");
  errorBox.classList.add("hidden");

  const bookingId = Number(document.getElementById("edit-id").value);
  const payload = {
    clientName: document.getElementById("edit-name").value.trim(),
    clientPhone: document.getElementById("edit-phone").value.trim(),
    day: Number(document.getElementById("edit-day").value),
    month: Number(document.getElementById("edit-month").value),
    year: Number(document.getElementById("edit-year").value),
    slot: Number(document.getElementById("edit-slot").value),
    hallId: Number(document.getElementById("edit-hall").value),
    guests: Number(document.getElementById("edit-guests").value),
    menuId: Number(document.getElementById("edit-menu").value),
    theme: document.getElementById("edit-theme").value,
    occasion: document.getElementById("edit-occasion").value.trim(),
    customRequest: document.getElementById("edit-request").value.trim(),
    customRequestStatus: document.getElementById("edit-request-status").value
  };

  try {
    await apiFetch(`/bookings/${bookingId}`, { method: "PUT", body: JSON.stringify(payload) });
    document.getElementById("edit-modal").classList.add("hidden");
    showToast(`Reservation #${bookingId} updated!`, "success");
    await Promise.all([loadBookings(), loadAvailabilityMatrix()]);
  } catch (err) {
    errorBox.textContent = err.message;
    errorBox.classList.remove("hidden");
  }
}

async function handleDeleteBooking(bookingId) {
  if (!confirm(`Delete Reservation #${bookingId}?`)) return;
  try {
    await apiFetch(`/bookings/${bookingId}`, { method: "DELETE" });
    showToast(`Reservation #${bookingId} deleted.`, "success");
    await Promise.all([loadBookings(), loadAvailabilityMatrix()]);
  } catch (err) {
    showToast(err.message, "error");
  }
}

// ==================== EVENTS & BOOTSTRAP ====================
function toggleSort(field) {
  if (appState.sortBy === field) {
    appState.sortOrder = appState.sortOrder === "asc" ? "desc" : "asc";
  } else {
    appState.sortBy = field;
    appState.sortOrder = "asc";
  }
  document.getElementById("sort-date-btn").classList.toggle("active", appState.sortBy === "date");
  document.getElementById("sort-cost-btn").classList.toggle("active", appState.sortBy === "cost");
  loadBookings();
}

function bindEvents() {
  // Navigation
  document.getElementById("nav-brand-home").addEventListener("click", (e) => {
    e.preventDefault();
    switchView("view-home");
  });
  document.getElementById("nav-home-btn").addEventListener("click", () => switchView("view-home"));
  document.getElementById("nav-all-halls-btn").addEventListener("click", () => switchView("view-all-halls"));
  document.getElementById("nav-matrix-btn").addEventListener("click", () => {
    loadAvailabilityMatrix();
    switchView("view-matrix");
  });
  document.getElementById("nav-reservations-btn").addEventListener("click", () => {
    switchView("view-home");
    setTimeout(() => document.getElementById("reservations-section").scrollIntoView({ behavior: "smooth" }), 50);
  });
  document.getElementById("nav-cta-btn").addEventListener("click", () => {
    switchView("view-home");
    setTimeout(() => document.getElementById("search-sector").focus(), 100);
  });
  document.getElementById("home-view-all-btn").addEventListener("click", () => switchView("view-all-halls"));

  document.querySelectorAll(".back-home-btn").forEach((b) =>
    b.addEventListener("click", () => switchView("view-home"))
  );
  document.getElementById("cfg-back-btn").addEventListener("click", () => switchView(appState.previousView));
  document.getElementById("chk-back-btn").addEventListener("click", () => switchView("view-configure"));

  // Home Search
  document.getElementById("search-month").addEventListener("change", () => syncDaysDropdown("search-day", "search-month", "search-year"));
  document.getElementById("search-year").addEventListener("change", () => syncDaysDropdown("search-day", "search-month", "search-year"));
  document.getElementById("home-search-form").addEventListener("submit", handleSearchHalls);

  document.querySelectorAll("#sector-chips .quick-date-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      document.getElementById("search-sector").value = btn.dataset.sector;
      document.querySelectorAll("#sector-chips .quick-date-btn").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
    });
  });

  // Configure page
  document.getElementById("cfg-month").addEventListener("change", () => {
    syncDaysDropdown("cfg-day", "cfg-month", "cfg-year");
    refreshConfigureSlots();
  });
  document.getElementById("cfg-year").addEventListener("change", () => {
    syncDaysDropdown("cfg-day", "cfg-month", "cfg-year");
    refreshConfigureSlots();
  });
  document.getElementById("cfg-day").addEventListener("change", refreshConfigureSlots);
  document.getElementById("cfg-guests").addEventListener("input", updateConfigureDiscountHint);

  document.querySelectorAll("#cfg-theme-selector .slot-option").forEach((btn) => {
    btn.addEventListener("click", () => {
      appState.theme = btn.dataset.theme;
      document.querySelectorAll("#cfg-theme-selector .slot-option").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
    });
  });

  document.querySelectorAll("#cfg-occasion-selector .slot-option").forEach((btn) => {
    btn.addEventListener("click", () => {
      appState.occasionMode = btn.dataset.occasion;
      document.querySelectorAll("#cfg-occasion-selector .slot-option").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
      document.getElementById("cfg-custom-occasion-wrap").classList.toggle("hidden", appState.occasionMode !== "Other");
      updateOccasionPerksBox();
    });
  });

  document.getElementById("cfg-custom-occasion").addEventListener("input", (e) => {
    appState.customOccasion = e.target.value;
    updateOccasionPerksBox();
  });

  document.getElementById("configure-form").addEventListener("submit", handleConfigureSubmit);

  // Checkout page
  document.getElementById("chk-client-phone").addEventListener("input", renderCheckoutSummaryAndBilling);
  document.getElementById("checkout-form").addEventListener("submit", handleFinalBooking);

  // Matrix page
  document.getElementById("matrix-month").addEventListener("change", () => {
    syncDaysDropdown("matrix-day", "matrix-month", "matrix-year");
    loadAvailabilityMatrix();
  });
  document.getElementById("matrix-year").addEventListener("change", () => {
    syncDaysDropdown("matrix-day", "matrix-month", "matrix-year");
    loadAvailabilityMatrix();
  });
  document.getElementById("matrix-day").addEventListener("change", loadAvailabilityMatrix);
  document.querySelectorAll(".matrix-qdate").forEach((btn) => {
    btn.addEventListener("click", () => {
      document.getElementById("matrix-month").value = btn.dataset.month;
      document.getElementById("matrix-year").value = btn.dataset.year;
      syncDaysDropdown("matrix-day", "matrix-month", "matrix-year", Number(btn.dataset.day));
      loadAvailabilityMatrix();
    });
  });

  // Bookings toolbar
  document.getElementById("bookings-search").addEventListener("input", loadBookings);
  document.getElementById("bookings-hall-filter").addEventListener("change", loadBookings);
  document.getElementById("refresh-bookings-btn").addEventListener("click", loadBookings);
  document.getElementById("sort-date-btn").addEventListener("click", () => toggleSort("date"));
  document.getElementById("sort-cost-btn").addEventListener("click", () => toggleSort("cost"));

  // Edit modal date & submit
  document.getElementById("edit-month").addEventListener("change", () => syncDaysDropdown("edit-day", "edit-month", "edit-year"));
  document.getElementById("edit-year").addEventListener("change", () => syncDaysDropdown("edit-day", "edit-month", "edit-year"));
  document.getElementById("edit-booking-form").addEventListener("submit", handleUpdateBooking);

  // Modals close
  document.querySelectorAll("[data-close-modal]").forEach((btn) => {
    btn.addEventListener("click", () => document.getElementById(btn.dataset.closeModal).classList.add("hidden"));
  });
  document.querySelectorAll(".modal-backdrop").forEach((backdrop) => {
    backdrop.addEventListener("click", (e) => {
      if (e.target === backdrop) backdrop.classList.add("hidden");
    });
  });

  document.getElementById("retry-connection-btn").addEventListener("click", bootstrapApp);
}

async function bootstrapApp() {
  try {
    await loadCatalog();
    await Promise.all([loadBookings(), loadAvailabilityMatrix()]);
  } catch (err) {
    showToast(err.message, "error");
  }
}

document.addEventListener("DOMContentLoaded", () => {
  populateDateSelects("search-day", "search-month", "search-year", 15, 11, 2026);
  populateDateSelects("cfg-day", "cfg-month", "cfg-year", 15, 11, 2026);
  populateDateSelects("matrix-day", "matrix-month", "matrix-year", 15, 11, 2026);
  populateDateSelects("edit-day", "edit-month", "edit-year", 15, 11, 2026);
  bindEvents();
  bootstrapApp();
});
