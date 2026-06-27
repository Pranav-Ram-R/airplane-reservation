# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

A command-line domestic airline reservation simulator written in C++17. Single executable, no external dependencies, no build system — compilation is a manual `g++` invocation. State is persisted to plain-text files in the working directory.

## Build & Run

There is no Makefile or CMake. Compile all sources together from the project root:

```bash
g++ -std=c++17 -Iinclude main.cpp src/*.cpp -o airline
./airline          # Linux/macOS
airline.exe        # Windows
```

Run the binary **from the directory containing `flights.txt`, `bookings.txt`, and `admin_credentials.txt`** — all file paths are relative to the current working directory.

There are no automated tests; verification is manual through the interactive menu.

### Toolchain constraints

- `main.cpp` includes `<bits/stdc++.h>`, which is **GCC-specific**. Use g++/MinGW, not MSVC or stock clang, unless you replace that include.
- `main.cpp` includes `"include/pricing.h"` (lowercase) while the file is `include/Pricing.h`. This compiles on Windows (case-insensitive FS) but **breaks on Linux/macOS** — fix the casing if building there.

## Architecture

`main.cpp` drives the entire flow: it loads files, runs a login gate, then either an admin panel or the passenger menu loop. All input validation helpers (`getValidatedInt`, `getValidatedChar`, `getValidatedAadhaar`, `clearInputStream`) live in `main.cpp`, not in a shared utility.

`BookingSystem` (include/src `BookingSystem`) is the central state container and the single source of truth. It owns:
- `flights` — `vector<Flight>`
- `bookings` — `flightID → vector<Passenger>`
- `bookedPassports`, `bookingHistory`, `passengerHistory` — keyed by flight or Aadhaar
- `passengerSeatMap` — `flightID → [(aadhaar, (row, col))]`, used to locate a passenger's seat for boarding passes
- `waitlist` — a `priority_queue<Passenger>` ordered by `AgeComparator` (older passengers first)
- `boardingQueue` — a FIFO `queue<Passenger>`

Nearly every other module reads/writes through a `BookingSystem` reference passed in.

### Module responsibilities

- **Flight** — seat inventory as `vector<vector<bool>>` (40 rows × 6 cols, A–F). Owns seat availability, booking, revenue, and occupancy-rate math.
- **Passenger** — passenger record. Note: the Aadhaar number is stored in the field named `passportNumber`.
- **LoginManager** — static `loginAsAdmin()` / `loginAsPassenger()`. Admin auth reads `admin_credentials.txt` (`username,password` per line, plaintext). Also declares free function `isValidAadhaar()` (12-digit check).
- **RouteManager** — generates synthetic flight options per `(src, dst)` route and `TimeSlot` (Morning/Afternoon/Evening), assigns flight IDs (`<Src><Dst><n>`), and `normalize()`s city names. Routes are generated in-memory at construction.
- **Pricing** — `Pricing::calculateFare(baseFare, totalSeats, bookedSeats)` (dynamic, occupancy-based) and free function `getBaseFare(src, dst)`.
- **Admin** — admin panel menu; operates on a `BookingSystem&` (occupancy, revenue, booking records, waitlist/queue inspection).
- **BoardingPass** — static `generate(passenger, flight, row, col)`; formatting only.
- **VoiceAssistant** — `speak(text)`, a cross-platform TTS shim invoking `system()`: PowerShell `System.Speech` on Windows, `say` on macOS, `espeak` on Linux. Called throughout `main.cpp` alongside `cout` output.

### Persistence format

CSV-like, comma-delimited, one record per line:
- `flights.txt`: `flightID,source,destination,date,time,rows,cols,baseFare`
- `bookings.txt`: written by `BookingSystem::saveDataToFile`, loaded by `loadDataFromFile`.

Flights are saved eagerly during the passenger flow (`saveFlightsToFile` is called inside the route-seeding loop); bookings are saved on logout and on admin-panel exit. When changing any record schema, update both the matching `save*` and `load*` methods in `BookingSystem.cpp` together.

## Conventions

- Headers in `include/`, implementations in `src/`, mirrored by name. Add a new feature as a paired `include/X.h` + `src/X.cpp` and a corresponding handler/menu branch in `main.cpp`.
- Seats are 1-indexed in the UI (rows 1–40, cols A–F) and 0-indexed internally; conversions happen in `main.cpp` (`row - 1`, `col - 'A'`). Preserve this boundary.
- `Flight`'s data members are public and accessed directly (e.g. `fl.flightID`); there are also getter methods for admin stats.
