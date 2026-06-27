#include "include/BookingSystem.h"
#include "include/Flight.h"
#include "include/Passenger.h"
#include "include/RouteManager.h"
#include "include/Admin.h"
#include "include/pricing.h"
#include "include/VoiceAssistant.h"
#include "include/BoardingPass.h"
#include "include/LoginManager.h"

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <bits/stdc++.h>   // GCC-only convenience header that pulls in the whole STL.
                           // Fine for learning/competitive code, but DISCOURAGED in
                           // production: non-portable (clang/MSVC lack it) and slows compiles.

// ---- Input-validation helpers (the app's defensive I/O layer) ----

// Recover cin after a bad/extra input: clear() resets error flags, ignore() discards
// the rest of the line up to '\n' so leftover characters don't poison the next read.
// This pairing is the canonical fix for the classic "cin >> int fed a letter" failure.
void clearInputStream() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Robust integer input: loops until the user enters a number within [min, max].
// Default args (min/max) let callers omit bounds. Pattern: read, validate, on failure
// reset the stream and re-prompt — the input never crashes the program.
int getValidatedInt(const std::string& prompt, int min = INT_MIN, int max = INT_MAX) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        // cin.fail() is true when extraction failed (non-numeric input). Combined with
        // the range check, this rejects both wrong-type and out-of-range entries.
        if (std::cin.fail() || value < min || value > max) {
            std::cout << "Invalid input. Enter a valid number";
            if (min != INT_MIN && max != INT_MAX)
                std::cout << " between " << min << " and " << max;
            std::cout << ".\n";
            speak("Invalid input.");
            clearInputStream();
        } else {
            clearInputStream();
            return value;
        }
    }
}

// Constrained char input: keeps asking until the char is in `validChars` (e.g. "MF",
// "ABCDEF"). toupper normalizes case so 'm' and 'M' both pass. find()==npos means
// "not present" (npos is string's sentinel for "no match").
char getValidatedChar(const std::string& prompt, const std::string& validChars) {
    char ch;
    while (true) {
        std::cout << prompt;
        std::cin >> ch;
        ch = toupper(ch);
        if (validChars.find(ch) != std::string::npos) {
            clearInputStream();
            return ch;
        } else {
            std::cout << "Invalid input. Enter one of: " << validChars << "\n";
            speak("Invalid input.");
            clearInputStream();
        }
    }
}

std::string getValidatedAadhaar() {
    std::string aadhaar;
    while (true) {
        std::cout << "Enter Aadhaar Number (12-digit number): ";
        std::cin >> aadhaar;
        if (!isValidAadhaar(aadhaar)) {
            std::cout << "Invalid Aadhaar. Must be 12 digits.\n";
            speak("Invalid Aadhaar.");
            clearInputStream();
        } else {
            return aadhaar;
        }
    }
}

// ===========================================================================
// main(): the application's CONTROLLER / orchestrator. High-level flow:
//   load persisted state -> authenticate (admin vs passenger) -> run that role's
//   menu loop -> persist state on exit. All business logic lives in the classes;
//   main only wires them together and drives the console UI.
// ===========================================================================
int main() {
    speak("Welcome to the Domestic Airline Booking System.");

    BookingSystem system;          // the shared state object everything operates on
    RouteManager routeManager;     // ctor pre-generates the flight catalogue
    std::string currentAadhaar;    // session key for the logged-in passenger
    int loginChoice;
    // Hydrate in-memory state from disk BEFORE anything else (order matters: load
    // flights first so loadDataFromFile can re-mark booked seats on those flights).
    system.loadFlightsFromFile("flights.txt");
    system.loadDataFromFile("bookings.txt");

    std::cout << "Login as:\n1. Admin\n2. Passenger\nEnter choice (1 or 2): ";
    std::cin >> loginChoice;
    clearInputStream();

    // Role-based dispatch: 1 => admin branch (and exit), 2 => passenger flow below.
    if (loginChoice == 1) {
        if (!LoginManager::loginAsAdmin()) {
            std::cout << "Admin login failed. Exiting.\n";
            speak("Admin login failed.");
            return 0;
        }
        std::cout << "Admin login successful.\n";
        speak("Admin login successful.");

        Admin admin;
        admin.showMenu(system); 
        system.saveDataToFile("bookings.txt"); // Go directly to admin panel
        return 0;
    }

    if (loginChoice != 2 || !LoginManager::loginAsPassenger(currentAadhaar)) {
        std::cout << "Passenger login failed. Exiting.\n";
        speak("Passenger login failed.");
        return 0;
    }
    std::cout << "Passenger login successful.\n";
    speak("Passenger login successful.");

    // Passenger flow
    std::string src, dst, date;
    std::cout << "Enter Source City: ";
    std::cin >> src;
    std::cout << "Enter Destination City: ";
    std::cin >> dst;
    std::cout << "Enter Date (YYYY-MM-DD): ";
    std::cin >> date;

    src = RouteManager::normalize(src);
    dst = RouteManager::normalize(dst);

    // Pull up-to-9 time-sorted flight options for the chosen route and materialize them
    // as real Flight objects (40 rows x 6 cols) inside the BookingSystem so they can be booked.
    auto flights = routeManager.getFlightOptionsByTime(src, dst, RouteManager::TimeSlot::Morning);
    if (flights.empty()) {
        std::cout << "No flights available.\n";
        return 0;
    }

    // std::min<size_t>(9, ...) caps the loop at 9 even if more options exist; the explicit
    // <size_t> avoids a signed/unsigned template type-mismatch on the two arguments.
    for (size_t i = 0; i < std::min<size_t>(9, flights.size()); ++i) {
        std::string id = flights[i].first;
        std::string time = flights[i].second;
        int baseFare = getBaseFare(src, dst);
        Flight flight(id, src, dst, date, time, 40, 6, baseFare);
        system.addFlight(flight);
        system.saveFlightsToFile("flights.txt");   // (re-saves every iteration — could be hoisted out of the loop)

    }

    // Passenger menu loop: classic console-app REREPL — print options, read a validated
    // choice, dispatch with if/else, repeat until Logout (case 6) breaks out.
    while (true) {
        std::cout << "\n--- MAIN MENU ---\n";
        std::cout << "1. View Flights\n";
        std::cout << "2. Book Ticket\n";
        std::cout << "3. Cancel Ticket\n";
        std::cout << "4. View Booking History\n";
        std::cout << "5. Generate Boarding Pass\n";
        std::cout << "6. Logout\n";

        int choice = getValidatedInt("Enter your choice (1-6): ", 1, 6);

        if (choice == 1) {
            system.listFlights();

        } else if (choice == 2) {
            std::string flightID;
            std::cout << "Enter Flight ID: ";
            std::cin >> flightID;

            // Linear search for the flight; `const Flight* f` is a non-owning pointer
            // left as nullptr to signal "not found" (cheaper than copying a Flight).
            const Flight* f = nullptr;
            for (auto& fl : system.getFlights()) {
                if (fl.flightID == flightID) {
                    f = &fl;
                    break;
                }
            }

            if (!f) {
                std::cout << "Flight not found.\n";
                speak("Flight not found.");
                continue;   // skip back to the menu without crashing
            }

            f->displaySeats();   // `->` because f is a pointer

            std::string tempName;
            int tempAge;
            char tempGender;
            int tempRow;
            char tempColChar;

            while (true) {
                std::cout << "Enter Passenger Name: ";
                std::cin >> tempName;
                tempAge = getValidatedInt("Enter Age: ", 1, 120);
                tempGender = getValidatedChar("Enter Gender (M/F): ", "MF");
                tempRow = getValidatedInt("Enter Seat Row (1-40): ", 1, 40);
                tempColChar = getValidatedChar("Enter Seat Column (A-F): ", "ABCDEF");

                std::string confirm;
                std::cout << "You entered:\n";
                std::cout << "  Name: " << tempName << "\n";
                std::cout << "  Age: " << tempAge << "\n";
                std::cout << "  Gender: " << tempGender << "\n";
                std::cout << "  Seat: " << tempRow << tempColChar << "\n";
                std::cout << "Confirm booking details? (yes/back): ";
                std::cin >> confirm;

                if (confirm == "yes") break;   // confirmation loop: lets the user re-enter before committing
                else std::cout << "Re-enter your details.\n";
            }

            // Convert UI seat (1-based row, letter col) to internal 0-based indices.
            // 'C' - 'A' == 2, so column letters map to 0..5. row-1 is applied below.
            int tempCol = toupper(tempColChar) - 'A';

            if (!f->isSeatAvailable(tempRow - 1, tempCol)) {
                std::cout << "Seat is already booked. Please choose another seat.\n";
                speak("Seat is already booked.");
                continue;
            }

            Passenger p(tempName, currentAadhaar, tempAge, tempGender);
            if (system.bookTicket(flightID, p, tempRow - 1, tempCol)) {
                std::cout << "Booking successful.\n";
                speak("Booking successful.");
            } else {
                std::cout << "Booking failed.\n";
                speak("Booking failed.");
            }

        } else if (choice == 3) {
            std::string flightID;
            std::cout << "Enter Flight ID to cancel: ";
            std::cin >> flightID;

            std::string confirm;
            std::cout << "Are you sure you want to cancel this ticket? (yes/no): ";
            std::cin >> confirm;
            if (confirm != "yes") {
                std::cout << "Cancellation aborted.\n";
                continue;
            }

            system.cancelTicket(flightID, currentAadhaar);

        } else if (choice == 4) {
            system.viewPassengerHistory(currentAadhaar);

        } else if (choice == 5) {
            std::string flightID;
            std::cout << "Enter Flight ID: ";
            std::cin >> flightID;

            const auto& flights = system.getFlights();
            const Flight* f = nullptr;
            for (const auto& fl : flights) {
                if (fl.flightID == flightID) {
                    f = &fl;
                    break;
                }
            }

            if (!f) {
                std::cout << "Flight not found.\n";
                continue;
            }

            // Boarding pass = join two lookups: the seat (from the seat map) and the
            // Passenger record (from bookings), then hand both to BoardingPass::generate.
            auto seat = system.getPassengerSeat(flightID, currentAadhaar);
            if (seat.first != -1 && seat.second != -1) {   // -1 sentinel => no booking on this flight
                const auto& bookings = system.getBookings().at(flightID);
                for (const auto& p : bookings) {
                    if (p.passportNumber == currentAadhaar) {
                        BoardingPass::generate(p, *f, seat.first, seat.second);  // *f dereferences the pointer
                        break;
                    }
                }
            } else {
                std::cout << "No boarding pass found.\n";
                speak("Boarding pass not found.");
            }

        } else if (choice == 6) {
            // Logout = the ACTUAL persistence point: flush all bookings to disk here,
            // then break the loop so main() returns. (Recall bookTicket's own save call
            // is dead code, so without this the session's bookings would be lost.)
            std::cout << "Logged out successfully. Goodbye!\n";
            speak("Logged out. Thank you.");
            system.saveDataToFile("bookings.txt");
            break;
        }
    }

    return 0;
}
