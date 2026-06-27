# Technical Guide — Domestic Airplane Reservation Simulator

A deep-dive reference for the C++17 command-line airline reservation system. This document explains **how the system is built**, **why each design decision was made**, and **how data flows** through it — aimed at someone reading the code for the first time, preparing for an interview about it, or extending it.

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Build & Run](#2-build--run)
3. [Architecture at a Glance](#3-architecture-at-a-glance)
4. [Module Reference](#4-module-reference)
5. [Core Data Structures & Why They Were Chosen](#5-core-data-structures--why-they-were-chosen)
6. [End-to-End Control Flow](#6-end-to-end-control-flow)
7. [Key Workflows (Sequence Walkthroughs)](#7-key-workflows-sequence-walkthroughs)
8. [Persistence Layer & File Formats](#8-persistence-layer--file-formats)
9. [Business Rules](#9-business-rules)
10. [Cross-Cutting Concerns](#10-cross-cutting-concerns)
11. [Known Issues & Technical Debt](#11-known-issues--technical-debt)
12. [Extension Guide](#12-extension-guide)
13. [Glossary of C++ Concepts Used](#13-glossary-of-c-concepts-used)

---

## 1. System Overview

The simulator models a domestic airline booking platform as a single-process, console-driven C++17 application. It supports two roles:

- **Passenger** — log in with a 12-digit Aadhaar, search a route, view a live seat map, book/cancel seats, view history, and generate a boarding pass.
- **Admin** — log in with username/password, then view flights, bookings, occupancy, and revenue reports for a route.

State persists between runs via plain-text files. There are **no external dependencies** — only the C++ standard library plus the OS's native text-to-speech tool.

| Property | Value |
|---|---|
| Language | C++17 |
| Paradigm | Object-oriented, modular (header/impl split) |
| UI | Console (text menus) |
| Persistence | Flat CSV-style text files |
| External libs | None (STL only) |
| Build system | None — manual `g++` invocation |
| Entry point | `main.cpp` |

---

## 2. Build & Run

There is no Makefile/CMake. Compile all translation units together:

```bash
g++ -std=c++17 -Iinclude main.cpp src/*.cpp -o airline
./airline          # Linux/macOS
airline.exe        # Windows
```

**Run from the directory containing the data files** (`flights.txt`, `bookings.txt`, `admin_credentials.txt`) — every path in the code is relative to the current working directory.

### Toolchain constraints

- `main.cpp` includes `<bits/stdc++.h>`, a **GCC-only** convenience header. Use g++ / MinGW / MSYS2, not stock MSVC or clang, unless you replace that include.
- `main.cpp` includes `"include/pricing.h"` (lowercase) but the file is `Pricing.h`. Compiles on Windows (case-insensitive FS); **breaks on Linux/macOS** — fix the casing there.
- Verified building cleanly with **g++ 15.2.0 (MSYS2 UCRT64)**.

There are no automated tests; verification is manual through the menus.

---

## 3. Architecture at a Glance

The application follows a loose **Controller → Domain → Persistence** layering, with `main()` as the controller wiring everything together. `BookingSystem` is the central state hub that nearly every other module operates on **by reference**.

```
                          ┌──────────────────────────────┐
                          │           main.cpp            │
                          │   (controller / UI driver)    │
                          │  input validation + menus     │
                          └───────────────┬──────────────┘
                                          │ owns & passes by reference
                  ┌───────────────────────┼───────────────────────┐
                  ▼                        ▼                        ▼
          ┌───────────────┐       ┌─────────────────┐      ┌───────────────┐
          │ LoginManager  │       │  BookingSystem  │◄─────│     Admin     │
          │ (auth gate)   │       │  (STATE HUB /   │ ref  │ (reports)     │
          └───────────────┘       │   single source │      └───────────────┘
                                  │   of truth)     │
          ┌───────────────┐       │                 │      ┌───────────────┐
          │ RouteManager  │──────►│  flights        │◄─────│ BoardingPass  │
          │ (flight gen)  │ feeds │  bookings       │ reads│ (ticket out)  │
          └───────────────┘       │  seatMap, etc.  │      └───────────────┘
                                  └────────┬────────┘
          ┌───────────────┐                │ uses               ┌───────────────┐
          │   Pricing     │◄───────────────┤                    │ VoiceAssistant│
          │ (fare policy) │                │                    │ (TTS, global) │
          └───────────────┘                ▼                    └───────────────┘
                                  ┌─────────────────┐
                                  │  Flight (entity)│
                                  │  + seatMatrix   │
                                  └─────────────────┘
                                          │ persists to / loads from
                                          ▼
                          flights.txt   bookings.txt   admin_credentials.txt
```

**Design principles in play:**

- **Single source of truth** — all runtime state lives in one `BookingSystem` instance; modules never keep their own copies of bookings.
- **Separation of concerns** — auth (LoginManager), pricing policy (Pricing), output formatting (BoardingPass), data generation (RouteManager), and platform I/O (VoiceAssistant) are each isolated.
- **Dependency injection by reference** — `Admin` and the menu code receive `BookingSystem&` rather than constructing or owning data.
- **Stateless utilities** — Pricing, LoginManager, BoardingPass, and `RouteManager::normalize` expose `static` methods used as pure functions.

---

## 4. Module Reference

Each module is a header (`include/X.h`) + implementation (`src/X.cpp`) pair.

### `BookingSystem` — the state hub
The aggregate that owns all runtime collections and the methods that mutate them. Everything funnels through here: booking, cancellation, history, seat lookup, and persistence. See [§5](#5-core-data-structures--why-they-were-chosen) for its containers.

**Key methods:**
| Method | Purpose | Complexity |
|---|---|---|
| `addFlight(f)` | Append a flight | O(1) amortized |
| `bookTicket(id, p, row, col)` | Commit a booking across all structures | O(n) find + O(1) ops |
| `cancelTicket(id, aadhaar)` | Remove a booking, compute refund | O(n) |
| `getPassengerSeat(id, aadhaar)` | Reverse seat lookup → `{row,col}` or `{-1,-1}` | O(1) map + O(k) scan |
| `viewPassengerHistory(aadhaar)` | Per-user booking report | O(total bookings) |
| `save/loadDataToFile` | Booking persistence | O(bookings) |
| `save/loadFlightsToFile` | Flight persistence | O(flights) |

### `Flight` — the core domain entity
Models one scheduled flight plus its **seat inventory** as a 2D grid (`vector<vector<bool>>`, 40×6). All members are public (transparent value object). Owns seat availability, booking, revenue, and occupancy math. All seat operations are O(1) direct grid indexing.

### `Passenger` — value object
A lightweight record (`name`, `passportNumber`, `age`, `gender`) copied freely into containers. **Naming caveat:** `passportNumber` actually stores the **Aadhaar** number and serves as the unique user key.

### `LoginManager` — authentication gate
All-static utility. `loginAsAdmin()` checks input against `admin_credentials.txt`; `loginAsPassenger(aadhaar&)` validates a 12-digit Aadhaar and returns it via out-parameter. Also declares the free function `isValidAadhaar()`.

### `RouteManager` — flight catalogue generator
At construction, pre-builds a catalogue of synthetic flights for every ordered city pair (10 cities → 90 routes × 9 flights). Provides time-sorted flight options and city-name normalization. Uses a `std::map` keyed on a `std::pair<string,string>` (works because `std::pair` has a built-in `operator<`).

### `Pricing` — fare policy
Stateless policy class. `calculateFare()` implements **dynamic/surge pricing** based on occupancy tiers; the free function `getBaseFare()` is a city-pair lookup table. See [§9](#9-business-rules).

### `Admin` — reporting controller
Holds no data; every method takes a `BookingSystem&`. `showMenu()` seeds flights for a queried route, filters to the session's relevant flights, then runs an interactive report loop (flights / bookings / occupancy / revenue).

### `BoardingPass` — output utility
Single static factory `generate()` that composes a formatted ticket once and emits it to **both** a per-passenger file and the console.

### `VoiceAssistant` — platform TTS abstraction
A single `speak(text)` free function. Compile-time `#ifdef` dispatch shells out to PowerShell `System.Speech` (Windows), `say` (macOS), or `espeak` (Linux) via `system()`.

---

## 5. Core Data Structures & Why They Were Chosen

All of `BookingSystem`'s state is private. The container choices are the heart of the "data-structure justification" story:

```cpp
std::vector<Flight> flights;                                  // (A)

std::map<std::string, std::vector<Passenger>> bookings;       // (B) flightID -> passengers
std::map<std::string, std::unordered_set<std::string>>
        bookedPassports;                                      // (C) flightID -> set of Aadhaars
std::map<std::string, std::vector<BookingRecord>>
        bookingHistory, passengerHistory;                     // (D) audit trails

std::unordered_map<std::string,
    std::vector<std::pair<std::string, std::pair<int,int>>>>
        passengerSeatMap;                                     // (E) flightID -> [(aadhaar,(row,col))]

std::map<std::string,
    std::priority_queue<Passenger, std::vector<Passenger>,
                        AgeComparator>> waitlist;             // (F)
std::map<std::string, std::queue<Passenger>> boardingQueue;   // (G)
```

| # | Container | Why this choice |
|---|---|---|
| A | `vector<Flight>` | Contiguous & cache-friendly; iterated far more than searched. Trade-off: lookup-by-ID is an **O(n)** linear scan. |
| B | `map<string, vector<Passenger>>` | Ordered (Red-Black tree), **O(log n)** lookup; keeps flights in sorted-ID order "for free" when listing. |
| C | `map<..., unordered_set<string>>` | `unordered_set` gives **O(1)** average duplicate detection ("has this Aadhaar already booked?"). |
| D | `map<string, vector<BookingRecord>>` | Per-key append-only audit log; `passengerHistory` is keyed by Aadhaar for a per-user trail. |
| E | `unordered_map<...>` | Only ever does direct key lookups (never ordered iteration), so hashing **O(1) avg** beats a tree. This index reconstructs a passenger's seat for boarding passes. |
| F | `priority_queue` + `AgeComparator` | Max-heap that surfaces the **oldest** waitlisted passenger first (priority boarding). **O(log n)** push/pop. |
| G | `queue<Passenger>` | FIFO models first-come-first-served boarding order. |

> **`AgeComparator` detail:** `priority_queue` takes its comparator as a **type** parameter, so the comparator is a functor (`struct` with `operator()`), not a function value. `operator()` returns `a.age < b.age`; because `priority_queue` is a max-heap w.r.t. this "less-than", the oldest passenger ends up on top.

> **Honest note:** `waitlist` and `boardingQueue` are wired into the data model but **not exercised** by the current menu flow — they demonstrate intended extensibility.

### The "parallel structures" invariant

A single booking touches **four** structures that must stay in sync:

1. `Flight::seatMatrix` — the seat flag flips to `true`
2. `bookings[flightID]` — the `Passenger` is appended
3. `passengerHistory[aadhaar]` — a timestamped `BookingRecord`
4. `passengerSeatMap[flightID]` — the `(aadhaar, (row,col))` index entry

`bookTicket()` and `loadDataFromFile()` both populate these consistently. (See [§11](#11-known-issues--technical-debt) for where `cancelTicket()` breaks this invariant.)

---

## 6. End-to-End Control Flow

`main()` is the orchestrator. Its high-level shape:

```
main()
 ├─ speak("Welcome…")
 ├─ load flights.txt        ← order matters: flights first…
 ├─ load bookings.txt       ← …so booked seats can be re-marked on those flights
 ├─ prompt: Admin (1) or Passenger (2)
 │
 ├─ if Admin:
 │    ├─ LoginManager::loginAsAdmin()         → fail ⇒ exit
 │    ├─ Admin().showMenu(system)             → report loop
 │    └─ saveDataToFile("bookings.txt"); exit
 │
 └─ if Passenger:
      ├─ LoginManager::loginAsPassenger(aadhaar) → fail ⇒ exit
      ├─ read src/dst/date; normalize cities
      ├─ RouteManager::getFlightOptionsByTime() → seed up to 9 Flights into system
      └─ MAIN MENU loop (validated choice 1–6):
           1 View Flights      → system.listFlights()
           2 Book Ticket       → seat map → collect+confirm details → bookTicket()
           3 Cancel Ticket     → confirm → cancelTicket() (computes refund)
           4 View History      → viewPassengerHistory()
           5 Boarding Pass     → getPassengerSeat() + find Passenger → BoardingPass::generate()
           6 Logout            → saveDataToFile("bookings.txt"); break
```

**Persistence timing is subtle:** the actual save of bookings happens on **logout** (choice 6) and on admin-panel exit — *not* inside `bookTicket()` (its own save call is unreachable dead code; see [§11](#11-known-issues--technical-debt)).

---

## 7. Key Workflows (Sequence Walkthroughs)

### 7.1 Booking a ticket (passenger, menu option 2)

```
User → main: enter Flight ID
main → BookingSystem.getFlights(): linear search for the Flight*  (nullptr ⇒ "not found")
main → Flight.displaySeats(): render the 40×6 grid ([X]=booked, [ ]=free)
main → user: collect name/age/gender/row/col via getValidatedInt / getValidatedChar
main → user: show summary, require "yes" to confirm (else re-enter loop)
main: convert UI seat → 0-based (row-1, col-'A')
main → Flight.isSeatAvailable(): bounds + free check
main → BookingSystem.bookTicket():
        ├─ Flight.bookSeat()                    (flip grid, bookedSeats++)
        ├─ bookings[flightID].push_back(p)
        ├─ Pricing.calculateFare(base, total, booked)   ← computed AFTER bookSeat, so
        │                                                 fare reflects new occupancy tier
        ├─ build timestamp via <chrono>/<iomanip>
        ├─ passengerHistory[aadhaar].push_back(record)
        ├─ passengerSeatMap[flightID].push_back({aadhaar,{row,col}})
        └─ display fare + seat map → return true
```

### 7.2 Cancelling a ticket (passenger, menu option 3)

```
main → user: confirm ("yes"/no)
main → BookingSystem.cancelTicket(flightID, aadhaar):
        ├─ bookings.find(flightID)              O(log n)
        ├─ locate passenger in the vector       O(k)
        ├─ Flight.bookedSeats--
        ├─ Pricing.calculateFare(...) → originalFare
        ├─ calculateRefund(flight.date, originalFare)  → tiered refund (see §9)
        ├─ print refund + cancellation fee
        └─ passengerList.erase(it)              (iterator then discarded — safe)
```
⚠️ This frees the seat *count* but **not** the `seatMatrix` cell or the `passengerSeatMap` entry — see [§11](#11-known-issues--technical-debt).

### 7.3 Generating a boarding pass (passenger, menu option 5)

```
main → BookingSystem.getPassengerSeat(flightID, aadhaar)  → {row,col} or {-1,-1}
main → BookingSystem.getBookings().at(flightID): find the matching Passenger
main → BoardingPass.generate(passenger, *flight, row, col):
        ├─ seat label = (row+1) + ('A'+col)         e.g. "12C"
        ├─ compose ticket text in an ostringstream
        ├─ write to BoardingPass_<aadhaar>_<flightID>.txt
        └─ echo to console + speak()
```

### 7.4 Admin reporting

```
Admin.showMenu(system):
  ├─ read src/dst/date; normalize
  ├─ RouteManager.getFlightOptionsByTime() → addFlight() ×9 into system
  ├─ filter system.getFlights() → relevantFlights (copy of this route/date)
  └─ loop:
       1 View Flights    : list relevantFlights
       2 View Bookings   : per flight, bookings.find() → list passengers + seat label
       3 Occupancy       : booked/total %  (double cast avoids integer truncation)
       4 Revenue         : count × getBaseFare()  (simplified — ignores surge pricing)
       5 Exit            : break
```

---

## 8. Persistence Layer & File Formats

Persistence is a **manual serialization** scheme: in-memory objects are flattened to comma-separated lines; loading reverses it. Saving opens files in truncate mode (full rewrite each time). Parsing uses the `stringstream` + `getline(ss, field, ',')` tokenization idiom; `std::stoi` converts numeric fields.

### `flights.txt`
```
flightID,source,destination,date,time,rows,cols,baseFare
```
Example:
```
HydAhm1,Hyderabad,Ahmedabad,2025-07-08,06:08,40,6,4000
```
Written by `saveFlightsToFile()`, read by `loadFlightsFromFile()` (which `emplace_back`s each `Flight`).

### `bookings.txt`
```
flightID,name,aadhaar,age,gender,row,col
```
Written by `saveDataToFile()`. `loadDataFromFile()` rebuilds `bookings`, `passengerSeatMap`, **and** re-marks the seat on the matching `Flight` so the loaded state is indistinguishable from a live booking.

### `admin_credentials.txt`
```
username,password
```
Plaintext, one admin per line. Read linearly by `LoginManager::loginAsAdmin()`.

> **Loading order requirement:** `main()` calls `loadFlightsFromFile()` *before* `loadDataFromFile()`, because the latter needs the flights to exist in order to re-mark their booked seats.

---

## 9. Business Rules

### Dynamic (surge) pricing — `Pricing::calculateFare`
Fare scales with how full the flight already is (yield management):

| Occupancy | Multiplier |
|---|---|
| < 30% | ×1.0 (base) |
| 30–60% | ×1.25 |
| 60–90% | ×1.5 |
| ≥ 90% | ×2.0 |

`occupancyRate = static_cast<double>(bookedSeats) / totalSeats` — the cast forces floating-point division (without it, integer division truncates to 0).

### Base fare — `getBaseFare(src, dst)`
A cascading if/else lookup table of bidirectional city-pair fares (₹3000 / ₹4500 / ₹6000 / ₹7500), defaulting to **₹4000**. ⚠️ See [§11](#11-known-issues--technical-debt): a case mismatch currently makes every route fall through to the default.

### Refund policy — `calculateRefund(flightDate, fare)`
Based on days remaining before departure (computed with `<ctime>`: `parseDate` → `mktime` → `difftime`):

| Days before flight | Refund |
|---|---|
| > 7 | 100% |
| 1–7 | 50% |
| 0 / past | 0% |

### Validation rules
- **Aadhaar:** exactly 12 characters, all digits (`std::all_of(..., ::isdigit)`).
- **Age:** 1–120. **Gender:** `M`/`F`. **Row:** 1–40. **Column:** `A`–`F`.
- Bad input never crashes: `getValidatedInt`/`getValidatedChar` loop until valid, using `cin.clear()` + `cin.ignore(...)` to recover the stream.

---

## 10. Cross-Cutting Concerns

### Seat indexing convention
- **UI**: 1-based rows (1–40), letter columns (A–F).
- **Internal**: 0-based (`row-1`, `col-'A'`).
- Conversions happen at the boundary in `main.cpp` and are reversed for display (`row+1`, `'A'+col`). Preserve this boundary when adding seat features.

### Voice feedback
`speak()` is interleaved with `cout` throughout. It's a **blocking** `system()` call — speech plays synchronously, pausing the program until the utterance finishes. On Windows the text is sanitized (double-quotes stripped) before being embedded in the PowerShell command.

### Input handling
The `clearInputStream` / `getValidatedInt` / `getValidatedChar` / `getValidatedAadhaar` helpers in `main.cpp` form the defensive I/O layer. `cin.fail()` detects type mismatches; the clear+ignore pairing is the canonical fix for the "fed a letter to `cin >> int`" failure.

---

## 11. Known Issues & Technical Debt

These are real and worth knowing (they double as good "what's wrong here?" interview material):

| # | Location | Issue | Impact |
|---|---|---|---|
| 1 | `BookingSystem::bookTicket` | `saveDataToFile()` sits **after** `return true` → **dead/unreachable code** | Bookings aren't persisted at booking time; only saved on logout. |
| 2 | `getBaseFare` vs `RouteManager::normalize` | `getBaseFare` compares **lowercase** city literals, but cities are normalized to **Title-case** before the call | Special fares never match; everything defaults to ₹4000. |
| 3 | `BookingSystem::cancelTicket` | Decrements `bookedSeats` but never resets `seatMatrix[row][col]` or removes the `passengerSeatMap` entry | Cancelled seat still shows `[X]` and can't be rebooked; breaks the parallel-structure invariant. |
| 4 | `RouteManager::normalize` | `result[0]` on an empty string is **undefined behaviour** | Crash/garbage on empty city input. |
| 5 | `RouteManager::getFlightOptionsByTime` | `slot` parameter is **ignored** (all flights generated in the Morning window) | Afternoon/Evening selection has no effect. |
| 6 | `waitlist`, `boardingQueue` | Declared in the data model but **never used** by any flow | Dead state. |
| 7 | `main.cpp` loop | `saveFlightsToFile()` is called **inside** the seeding loop (re-saves every iteration) | Redundant I/O; could be hoisted out. |
| 8 | `BoardingPass.cpp` | Duplicate `#include` block (harmless due to guards) | Clutter. |
| 9 | `LoginManager` | Passwords stored/compared in **plaintext** | Security weakness (would use salted hash + constant-time compare in production). |
| 10 | `<bits/stdc++.h>` + lowercase `pricing.h` include | Non-portable | Won't build on non-GCC / case-sensitive filesystems without edits. |

---

## 12. Extension Guide

**Add a new feature/module:**
1. Create `include/X.h` (with include guard or `#pragma once`) and `src/X.cpp`.
2. Operate on the shared `BookingSystem&` rather than holding your own state.
3. Add a menu branch in `main.cpp` (or `Admin::showMenu` for admin features).
4. Recompile with the same `g++ -std=c++17 -Iinclude main.cpp src/*.cpp` command.

**Change a persisted record's schema:** update the matching `save*` **and** `load*` methods in `BookingSystem.cpp` together, or old files won't parse.

**Add seat state beyond booked/free** (e.g., held/blocked): the seat grid is `vector<vector<bool>>`; you'd promote it to an enum grid and update `isSeatAvailable`, `bookSeat`, and `displaySeats`.

**Wire up the waitlist:** when `bookTicket` finds no free seat, push the `Passenger` onto `waitlist[flightID]`; on cancellation, pop the highest-priority waitlisted passenger and assign the freed seat (which also requires fixing issue #3).

---

## 13. Glossary of C++ Concepts Used

| Concept | Where it appears | One-liner |
|---|---|---|
| Member-initializer list | `Flight`, `Passenger` ctors | Initializes members directly (avoids default-construct-then-assign); follows declaration order. |
| Include guard / `#pragma once` | every header | Prevents double-inclusion in a translation unit. |
| `static` member functions | LoginManager, Pricing, BoardingPass, `normalize` | Class-scoped utilities; no instance needed. |
| Functor (function object) | `AgeComparator` | A `struct` with `operator()` used as a comparator **type** for `priority_queue`. |
| `enum class` | `RouteManager::TimeSlot` | Scoped, type-safe enum (no implicit int conversion). |
| `const` ref params / methods | throughout | Pass-by-reference without copying; `const` promises no mutation. |
| Out-parameter | `loginAsPassenger(string&)` | Returns the validated Aadhaar via a non-const reference. |
| Sentinel return | `getPassengerSeat` → `{-1,-1}`, `findPassenger` → `nullptr` | Encodes "not found" without exceptions/optional. |
| `emplace_back` vs `push_back` | `loadFlightsFromFile` | Constructs in place, avoiding a temporary + copy. |
| `vector<bool>` | `Flight::seatMatrix` | Space-optimized bit-packed specialization (a known C++ gotcha). |
| STL algorithms | `std::find_if`, `std::all_of`, `std::sort`, `std::min` | Generic operations with predicate lambdas. |
| `<chrono>` / `<ctime>` | timestamps, refund math | Time capture and date arithmetic. |
| Stream tokenization | all file loaders | `stringstream` + `getline(ss, field, ',')` to parse CSV. |
| Compile-time `#ifdef` dispatch | `VoiceAssistant` | Picks one platform branch at compile time. |

---

*This guide reflects the codebase as documented inline in the headers and sources. When behavior and this document disagree, trust the code — and update this file.*
