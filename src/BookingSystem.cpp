#include "../include/BookingSystem.h"
#include "../include/Pricing.h"
#include "../include/VoiceAssistant.h"
#include "../include/Flight.h"
#include "../include/Pricing.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <ctime>

// Utility to parse a "YYYY-MM-DD" string into a std::tm (broken-down calendar time).
// std::get_time is the <iomanip> parser; `std::tm tm = {}` zero-inits all fields first
// so unspecified components (hour/min/sec) default to 0 instead of garbage.
std::tm parseDate(const std::string& dateStr) {
    std::tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    return tm;
}

// Refund policy engine: how much money the passenger gets back on cancellation,
// based on how many days remain before departure. Demonstrates <ctime> date math.
double calculateRefund(const std::string& flightDate, double originalFare) {
    std::tm flight_tm = parseDate(flightDate);
    std::time_t now = std::time(nullptr);             // current wall-clock time (epoch seconds)
    std::tm* current_tm = std::localtime(&now);

    // mktime converts broken-down tm -> time_t (epoch seconds) so we can subtract them.
    std::time_t flight_time = std::mktime(&flight_tm);
    std::time_t current_time = std::mktime(current_tm);

    double secondsLeft = std::difftime(flight_time, current_time);
    int daysLeft = static_cast<int>(secondsLeft / (60 * 60 * 24));  // seconds -> whole days

    // Tiered refund rule (the actual business policy):
    if (daysLeft > 7) return originalFare;            // >1 week out: full refund
    else if (daysLeft >= 1) return originalFare * 0.5; // within a week: 50%
    else return 0.0;                                   // day-of / past: no refund
}

void BookingSystem::addFlight(const Flight &f) {
    flights.push_back(f);
}

void BookingSystem::listFlights() const {
    std::cout << "\nAvailable Flights:\n";
    for (const auto &f : flights) {
        std::cout << "ID: " << f.flightID
                  << " | From: " << f.source
                  << " | To: " << f.destination
                  << " | Date: " << f.date
                  << " | Time: " << f.time << "\n";
    }
}

// Core booking transaction. Returns true on success, false otherwise.
// Complexity: O(n) to find the flight (linear scan of the `flights` vector) + O(1) seat ops.
bool BookingSystem::bookTicket(const std::string &flightID, const Passenger &p, int row, int col) {
    for (auto &f : flights) {            // `auto&` = reference, so we mutate the real Flight, not a copy
        if (f.flightID == flightID) {
            if (f.isSeatAvailable(row, col)) {
                // --- Commit the booking across all the parallel data structures ---
                f.bookSeat(row, col);                 // 1) flip the seat in the grid
                bookings[flightID].push_back(p);      // 2) record the passenger (map auto-creates the vector)

                // Dynamic pricing is computed AFTER bookSeat(), so bookedSeats already
                // includes this seat — the fare reflects the new occupancy tier.
                int baseFare = getBaseFare(f.source, f.destination);
                double finalFare = Pricing::calculateFare(baseFare, f.totalSeats, f.bookedSeats);

                // Build a "YYYY-MM-DD HH:MM:SS" timestamp from the system clock.
                auto now = std::chrono::system_clock::now();
                std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&timeNow), "%Y-%m-%d %H:%M:%S");

                BookingRecord record{flightID, ss.str()};
                passengerHistory[p.passportNumber].push_back(record);            // 3) audit trail (by Aadhaar)
                passengerSeatMap[flightID].push_back({ p.passportNumber, {row, col} }); // 4) seat lookup index

                std::cout << "Ticket Fare: ₹" << finalFare << "\n";
                speak("Your ticket fare is rupees " + std::to_string((int)finalFare));
                f.displaySeats();
                return true;
                // BUG / DEAD CODE (good interview "what's wrong here?" example):
                // this line is UNREACHABLE — it sits after `return true`, so bookings
                // are never persisted here. Persistence actually happens on logout in main().
                saveDataToFile("bookings.txt");

            } else {
                std::cout << "Seat already booked.\n";
                return false;
            }
        }
    }
    std::cout << "Flight not found.\n";
    return false;
}

// Cancel a booking and compute the refund.
// .find() on a map is O(log n); then linear scans to locate the passenger and flight.
void BookingSystem::cancelTicket(const std::string &flightID, const std::string &passportNumber) {
    auto it = bookings.find(flightID);          // iterator into the map; ==end() means key absent
    if (it != bookings.end()) {
        auto &passengerList = it->second;       // it->second is the vector<Passenger> for this flight
        for (auto pIt = passengerList.begin(); pIt != passengerList.end(); ++pIt) {
            if (pIt->passportNumber == passportNumber) {
                for (auto &f : flights) {
                    if (f.flightID == flightID) {
                        f.bookedSeats--;        // free one seat from the count

                        double originalFare = Pricing::calculateFare(
                            getBaseFare(f.source, f.destination),
                            f.totalSeats,
                            f.bookedSeats
                        );

                        double refund = calculateRefund(f.date, originalFare);
                        std::cout << "Refund Amount: ₹" << refund << "\n";
                        std::cout << "Cancellation Fee: ₹" << originalFare - refund << "\n";

                        // erase() removes the element and returns the next iterator; we
                        // immediately return, so invalidating `pIt` here is safe.
                        passengerList.erase(pIt);
                        // KNOWN GAP (talking point): this frees the seat COUNT but does
                        // NOT reset seatMatrix[row][col] back to false, nor remove the
                        // entry from passengerSeatMap. So the seat still shows as [X].
                        // A complete fix would clear all four structures in sync.
                        return;
                    }
                }
            }
        }
    }
    std::cout << "Ticket not found.\n";
}

void BookingSystem::viewPassengerHistory(const std::string& passportNumber) const {
    std::cout << "\n--- Booking History for Aadhaar: " << passportNumber << " ---\n";
    bool found = false;

    for (const auto& booking : bookings) {
        const std::string& flightID = booking.first;
        const auto& passengerList = booking.second;

        for (const auto& p : passengerList) {
            if (p.passportNumber == passportNumber) {
                found = true;

                // std::find_if + a lambda predicate = idiomatic STL "search for first
                // element matching a condition". `[&]` captures flightID by reference.
                // Returns flights.end() if nothing matches.
                auto it = std::find_if(flights.begin(), flights.end(), [&](const Flight& f) {
                    return f.flightID == flightID;
                });

                if (it != flights.end()) {
                    const Flight& f = *it;
                    auto seat = getPassengerSeat(flightID, passportNumber);
                    char seatCol = 'A' + seat.second;
                    std::cout << "Flight ID: " << f.flightID
                              << " | Route: " << f.source << " -> " << f.destination
                              << " | Date: " << f.date
                              << " | Time: " << f.time
                              << " | Seat: " << (seat.first + 1) << seatCol << "\n";
                }
            }
        }
    }

    if (!found) {
        std::cout << "No bookings found for this Aadhaar.\n";
    }
}

void BookingSystem::viewBookingHistory(const std::string& passportNumber) const {
    auto it = passengerHistory.find(passportNumber);
    if (it == passengerHistory.end() || it->second.empty()) {
        std::cout << "No booking history found for this Aadhaar.\n";
        return;
    }

    std::cout << "\nBooking History:\n";
    for (const auto& record : it->second) {
        std::cout << "Flight ID: " << record.flightID << " | Booked On: " << record.bookingTime << "\n";
    }
}

// Returns a pointer to the live Passenger inside `bookings` (so callers can mutate it),
// or nullptr if not found. Returning &p of a vector element is fine here because the
// vector isn't resized while the pointer is in use (otherwise it could dangle).
Passenger* BookingSystem::findPassenger(const std::string& flightID, const std::string& aadhaar) {
    auto it = bookings.find(flightID);
    if (it != bookings.end()) {
        for (auto& p : it->second) {
            if (p.passportNumber == aadhaar) {
                return &p;
            }
        }
    }
    return nullptr;
}

// Look up a passenger's {row, col} from the seat index. Returns the {-1,-1} sentinel
// when absent — callers test `seat.first != -1` instead of throwing/optional.
std::pair<int, int> BookingSystem::getPassengerSeat(const std::string& flightID, const std::string& aadhaar) const {
    auto it = passengerSeatMap.find(flightID);
    if (it != passengerSeatMap.end()) {
        for (const auto& pair : it->second) {   // linear scan of that flight's seat entries
            if (pair.first == aadhaar) {
                return pair.second;
            }
        }
    }
    return {-1, -1};
}

const std::unordered_map<std::string, std::vector<std::pair<std::string, std::pair<int, int>>>>& BookingSystem::getPassengerSeatMap() const {
    return passengerSeatMap;
}

// --- PERSISTENCE (serialization) ---
// Strategy: flatten in-memory objects to a flat CSV text file so state survives between
// runs (poor-man's database). Each save fully REWRITES the file (truncates on open).
// Format per line: flightID,name,aadhaar,age,gender,row,col
void BookingSystem::saveDataToFile(const std::string& filename) const {
    std::ofstream out(filename);            // opening in default (truncate) mode clears old contents
    if (!out.is_open()) {
        std::cerr << "Error: Unable to open file for writing.\n";
        return;
    }

    for (auto it = bookings.begin(); it != bookings.end(); ++it) {
        const std::string& flightID = it->first;
        const std::vector<Passenger>& passengers = it->second;

        for (const auto& p : passengers) {
            std::pair<int, int> seat = getPassengerSeat(flightID, p.passportNumber);
            out << flightID << "," << p.name << "," << p.passportNumber << "," << p.age << "," << p.gender
                << "," << seat.first << "," << seat.second << "\n";
        }
    }

    out.close();
}


// Load (deserialize) bookings back into memory at startup — the inverse of saveDataToFile.
// Silently returns if the file doesn't exist yet (first run). Parsing pattern:
// wrap each line in a stringstream, then getline(ss, field, ',') to split on commas.
void BookingSystem::loadDataFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return;          // no file => nothing to load (not an error)

    std::string line;
    while (std::getline(in, line)) {    // read file line by line
        std::stringstream ss(line);
        std::string flightID, name, aadhaar, ageStr, genderStr, rowStr, colStr;
        std::getline(ss, flightID, ',');   // ',' delimiter tokenizes the CSV record
        std::getline(ss, name, ',');
        std::getline(ss, aadhaar, ',');
        std::getline(ss, ageStr, ',');
        std::getline(ss, genderStr, ',');
        std::getline(ss, rowStr, ',');
        std::getline(ss, colStr, ',');

        // std::stoi = string -> int. (Assumes well-formed input; would throw on garbage.)
        int age = std::stoi(ageStr);
        char gender = genderStr[0];
        int row = std::stoi(rowStr);
        int col = std::stoi(colStr);

        // Rebuild the SAME parallel structures that bookTicket() populates, so the loaded
        // state is indistinguishable from a live booking.
        Passenger p(name, aadhaar, age, gender);
        bookings[flightID].push_back(p);
        passengerSeatMap[flightID].push_back({ aadhaar, {row, col} });

        // Re-mark the seat as taken on the matching Flight so the grid stays in sync.
        for (auto& f : flights) {
            if (f.flightID == flightID) {
                f.bookSeat(row, col);
                break;
            }
        }
    }
    in.close();
}
void BookingSystem::saveFlightsToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return;

    for (const auto& f : flights) {
        out << f.flightID << "," << f.source << "," << f.destination << ","
            << f.date << "," << f.time << "," << f.rows << "," << f.cols << "," << f.baseFare << "\n";
    }
    out.close();
}
void BookingSystem::loadFlightsFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string id, src, dst, date, time, rowsStr, colsStr, fareStr;
        std::getline(ss, id, ',');
        std::getline(ss, src, ',');
        std::getline(ss, dst, ',');
        std::getline(ss, date, ',');
        std::getline(ss, time, ',');
        std::getline(ss, rowsStr, ',');
        std::getline(ss, colsStr, ',');
        std::getline(ss, fareStr, ',');

        int rows = std::stoi(rowsStr);
        int cols = std::stoi(colsStr);
        int fare = std::stoi(fareStr);

        // emplace_back CONSTRUCTS the Flight in place inside the vector from these args,
        // avoiding the temporary-then-copy that push_back(Flight(...)) would incur.
        flights.emplace_back(id, src, dst, date, time, rows, cols, fare);
    }
    in.close();
}


