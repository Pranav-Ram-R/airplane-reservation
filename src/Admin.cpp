#include "../include/Admin.h"
#include "../include/RouteManager.h"
#include "../include/VoiceAssistant.h"
#include "../include/Pricing.h"
#include <iostream>
#include <iomanip>

// Admin entry point. Note the design: Admin holds NO data of its own — it receives a
// BookingSystem& and reads/seeds everything through that single shared object
// (dependency injection by reference). The method does three things in sequence:
//   1) seed flights for the queried route, 2) filter to this session's relevant flights,
//   3) run an interactive report menu over them.
void Admin::showMenu(BookingSystem& system) {
    std::string src, dst, date;
    std::cout << "Enter Source City: ";
    std::cin >> src;
    std::cout << "Enter Destination City: ";
    std::cin >> dst;
    std::cout << "Enter Date (YYYY-MM-DD): ";
    std::cin >> date;

    src = RouteManager::normalize(src);
    dst = RouteManager::normalize(dst);

    // Step 1: Generate 9 sorted flights using RouteManager
    RouteManager routeManager;
    auto flightsByTime = routeManager.getFlightOptionsByTime(src, dst, RouteManager::TimeSlot::Morning);

    if (flightsByTime.empty()) {
        std::cout << "No flights available for this route.\n";
        speak("No flights found.");
        return;
    }

    int baseFare = getBaseFare(src, dst);

    std::cout << "\nAvailable Flights for " << src << " to " << dst << " on " << date << ":\n";
    for (size_t i = 0; i < std::min<size_t>(9, flightsByTime.size()); ++i) {
        std::string id = flightsByTime[i].first;
        std::string time = flightsByTime[i].second;
        std::cout << "  Flight ID: " << id << " | Time: " << time << "\n";

        Flight f(id, src, dst, date, time, 40, 6, baseFare);
        system.addFlight(f);
    }

    // Step 2: Filter flights for this admin session.
    // getFlights() returns a const& (no copy); we then copy only the matching flights
    // into a local working set so the menu below iterates just this route/date.
    const auto& allFlights = system.getFlights();
    std::vector<Flight> relevantFlights;
    for (const auto& f : allFlights) {
        if (f.source == src && f.destination == dst && f.date == date) {
            relevantFlights.push_back(f);   // snapshot copy for this session
        }
    }

    // Step 3: Admin panel loop
    while (true) {
        std::cout << "\n--- ADMIN PANEL ---\n";
        std::cout << "1. View Flights\n";
        std::cout << "2. View All Bookings\n";
        std::cout << "3. Show Occupancy Stats\n";
        std::cout << "4. Show Revenue Report\n";
        std::cout << "5. Exit Admin Panel\n";

        int choice;
        std::cout << "Enter your choice (1-5): ";
        std::cin >> choice;

        if (choice == 1) {
            std::cout << "\nFlights Found:\n";
            for (const auto& f : relevantFlights) {
                std::cout << "ID: " << f.flightID
                          << " | Time: " << f.time
                          << " | Source: " << f.source
                          << " | Destination: " << f.destination
                          << " | Date: " << f.date << "\n";
            }
        }

        else if (choice == 2) {   // View all bookings, per flight
            for (const auto& f : relevantFlights) {
                const auto& bookings = system.getBookings();
                auto it = bookings.find(f.flightID);   // O(log n) map lookup
                if (it != bookings.end() && !it->second.empty()) {
                    std::cout << "\nFlight ID: " << f.flightID << "\n";
                    for (const auto& p : it->second) {
                        auto seat = system.getPassengerSeat(f.flightID, p.passportNumber);
                        // Ternary: format "12C" if a seat was found, else "N/A".
                        // seat.first != -1 tests against the {-1,-1} not-found sentinel.
                        std::string seatStr = (seat.first != -1)
                            ? std::to_string(seat.first + 1) + static_cast<char>('A' + seat.second)
                            : "N/A";

                        std::cout << "  Name: " << p.name
                                  << ", Aadhaar: " << p.passportNumber
                                  << ", Age: " << p.age
                                  << ", Gender: " << p.gender
                                  << ", Seat: " << seatStr << "\n";
                    }
                } else {
                    std::cout << "\nFlight ID: " << f.flightID << " has no bookings.\n";
                }
            }
        }

        else if (choice == 3) {   // Occupancy stats
            for (const auto& f : relevantFlights) {
                const auto& bookings = system.getBookings();
                // .count() returns 0/1 for a map (key present?); guards .at() which would
                // throw std::out_of_range on a missing key. (operator[] isn't usable on a
                // const map.) Cast to double so the percentage isn't integer-truncated.
                int booked = bookings.count(f.flightID) ? bookings.at(f.flightID).size() : 0;
                int total = f.getTotalSeats();
                double occupancy = (double)booked / total * 100.0;

                std::cout << "\nFlight ID: " << f.flightID
                          << "\n  Booked: " << booked
                          << " / " << total
                          << " (" << std::fixed << std::setprecision(2) << occupancy << "%)\n";
            }
        }

        else if (choice == 4) {   // Revenue report
            for (const auto& f : relevantFlights) {
                const auto& bookings = system.getBookings();
                int count = bookings.count(f.flightID) ? bookings.at(f.flightID).size() : 0;
                // Simplified revenue = headcount * base fare (ignores dynamic surge pricing
                // applied at booking time — a known simplification of the report).
                int fare = getBaseFare(f.source, f.destination);
                int revenue = count * fare;

                std::cout << "\nFlight ID: " << f.flightID
                          << "\n  Bookings: " << count
                          << "\n  Base Fare: ₹" << fare
                          << "\n  Revenue: ₹" << revenue << "\n";
            }
        }

        else if (choice == 5) {
            std::cout << "Exiting admin panel.\n";
            speak("Exiting admin panel.");
            break;
        }

        else {
            std::cout << "Invalid choice.\n";
        }
    }
}
