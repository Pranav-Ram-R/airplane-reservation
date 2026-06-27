#ifndef BOOKINGSYSTEM_H   // Include guard: prevents this header being compiled twice
#define BOOKINGSYSTEM_H   // in one translation unit (would cause "redefinition" errors).

#include "Flight.h"
#include "Passenger.h"
#include <vector>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <string>
#include <utility>

// Plain-Old-Data struct used as a value type: records WHEN a booking happened.
// A struct (vs class) signals "just grouped data, no invariants to protect".
struct BookingRecord {
    std::string flightID;
    std::string bookingTime;   // human-readable timestamp ("YYYY-MM-DD HH:MM:SS")
};

// BookingSystem is the "central hub" / single source of truth for all runtime state.
// Interview framing: this is the Facade-like aggregate that every other module
// (Admin, main menu, BoardingPass lookups) talks to via a reference. Encapsulation:
// all containers are private; outside code mutates them only through public methods.
class BookingSystem {
private:
    // ---- WHY EACH CONTAINER WAS CHOSEN (classic DS-justification interview Q) ----

    std::vector<Flight> flights;   // contiguous, cache-friendly; iterated far more than searched.
                                   // Trade-off: lookup-by-ID is O(n) linear scan (see bookTicket).

    // flightID -> passengers on that flight. std::map = ordered (Red-Black tree),
    // O(log n) lookup, keeps flights listed in sorted ID order "for free".
    std::map<std::string, std::vector<Passenger>> bookings;

    // flightID -> set of Aadhaar numbers already booked. unordered_set gives O(1)
    // average duplicate-detection. (Declared for "no double booking" guard.)
    std::map<std::string, std::unordered_set<std::string>> bookedPassports;

    std::map<std::string, std::vector<BookingRecord>> bookingHistory;    // keyed by flightID
    std::map<std::string, std::vector<BookingRecord>> passengerHistory;  // keyed by Aadhaar (per-user audit trail)

    // flightID -> [(aadhaar, (row, col))]. unordered_map: we only ever do direct
    // key lookups here, never ordered iteration, so hashing (O(1) avg) beats a tree.
    // This is the lookup that lets us reconstruct a passenger's seat for boarding passes.
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::pair<int, int>>>> passengerSeatMap;

    // Functor (function object): a struct with operator() used as a custom comparator.
    // Interview note: priority_queue takes the comparator as a TYPE parameter, so it
    // must be a type (functor/lambda-wrapped), not a plain function value.
    struct AgeComparator {
        bool operator()(const Passenger& a, const Passenger& b) {
            // priority_queue is a MAX-heap w.r.t. this "less-than": the element for
            // which everything else compares "<" sits on top. a.age < b.age => the
            // OLDEST passenger surfaces first (priority boarding for seniors).
            return a.age < b.age;
        }
    };

    // flightID -> max-heap of waitlisted passengers (oldest served first). O(log n) push/pop.
    std::map<std::string, std::priority_queue<Passenger, std::vector<Passenger>, AgeComparator>> waitlist;
    // flightID -> FIFO boarding line. queue models "first-come-first-served" boarding order.
    std::map<std::string, std::queue<Passenger>> boardingQueue;
    // NOTE (honest talking point): waitlist & boardingQueue are wired into the data model
    // but not exercised by the current menu flow — they show intended extensibility.

public:
    // --- Core Passenger Functionalities ---
    // const-ref params (const Flight&) = pass by reference to avoid copying, `const`
    // promises we won't mutate the caller's object. Standard C++ "cheap + safe" idiom.
    void addFlight(const Flight& f);
    void listFlights() const;   // trailing `const` = method does not modify *this (read-only)
    bool bookTicket(const std::string& flightID, const Passenger& p, int row, int col);
    void cancelTicket(const std::string& flightID, const std::string& passportNumber);

    // --- Passenger History & Search ---
    void viewPassengerHistory(const std::string& passportNumber) const;
    void viewBookingHistory(const std::string& passportNumber) const;
    // Returns a raw pointer so "not found" can be expressed as nullptr; non-owning
    // (caller must NOT delete it — it points into the `bookings` vector).
    Passenger* findPassenger(const std::string& flightID, const std::string& aadhaar);

    // --- Seat Info for Boarding Pass ---
    // Returns {row, col}; {-1,-1} is the sentinel for "passenger not on this flight".
    std::pair<int, int> getPassengerSeat(const std::string& flightID, const std::string& aadhaar) const;
    const std::unordered_map<std::string, std::vector<std::pair<std::string, std::pair<int, int>>>>& getPassengerSeatMap() const;

    // --- Admin Accessors ---
    // Returning `const&` exposes internals read-only without copying the whole vector/map.
    // Defined inline in the header so the compiler can inline these trivial getters.
    const std::vector<Flight>& getFlights() const { return flights; }
    const std::map<std::string, std::vector<Passenger>>& getBookings() const { return bookings; }
    const std::map<std::string, std::queue<Passenger>>& getBoardingQueue() const { return boardingQueue; }
    const std::map<std::string, std::priority_queue<Passenger, std::vector<Passenger>, AgeComparator>>& getWaitlist() const { return waitlist; }

    // --- File Persistence ---
    void saveDataToFile(const std::string& filename) const;
    void loadDataFromFile(const std::string& filename);
    void saveFlightsToFile(const std::string& filename) const;   // ✅ Added
    void loadFlightsFromFile(const std::string& filename);       // ✅ Added
};

#endif
