#include "../include/RouteManager.h"
#include "../include/VoiceAssistant.h"
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>

RouteManager::RouteManager() {
    generateRoutes();
}

// Canonicalize a city name to Title-case: "MUMBAI"/"mumbai" -> "Mumbai".
// Makes user input case-insensitive for route lookups.
// EDGE CASE (interview-honest): result[0] on an empty string is undefined behaviour —
// a production version would guard `if (city.empty()) return city;` first.
// SUBTLE BUG worth knowing: this yields Title-case, but getBaseFare() in Pricing.cpp
// compares against lowercase literals ("mumbai"), so the special fares there never
// match post-normalize and every route falls through to the ₹4000 default.
std::string RouteManager::normalize(const std::string &city) {
    std::string result = city;
    result[0] = toupper(result[0]);                       // first char upper
    for (size_t i = 1; i < result.size(); ++i)
        result[i] = tolower(result[i]);                   // rest lower
    return result;
}

std::string RouteManager::generateRandomTime(TimeSlot slot) {
    int startHour, endHour;
    switch (slot) {
        case TimeSlot::Morning:
            startHour = 6; endHour = 11;
            break;
        case TimeSlot::Afternoon:
            startHour = 12; endHour = 17;
            break;
        case TimeSlot::Evening:
            startHour = 18; endHour = 22;
            break;
    }

    // rand() % range maps a random int into [startHour, endHour]. (rand() is a simple
    // PRNG; <random>'s mt19937 + distributions would be the modern, less-biased choice.)
    int hour = startHour + rand() % (endHour - startHour + 1);
    int minute = rand() % 60;

    // Format as zero-padded "HH:MM". setw(2)+setfill('0') => "06" not "6".
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hour << ":"
        << std::setw(2) << std::setfill('0') << minute;
    return oss.str();
}

// Pre-build the entire flight catalogue once at construction.
// Seeds every ordered (src, dst) pair (excluding src==dst) with 9 flights each.
// Total complexity: O(cities^2 * 9). For 10 cities => 90 routes * 9 = 810 flights.
void RouteManager::generateRoutes() {
    srand(static_cast<unsigned>(time(nullptr)));   // seed PRNG with current time => varied times each run
    std::vector<std::string> cities = {
        "Mumbai", "Delhi", "Chennai", "Kolkata", "Bengaluru",
        "Hyderabad", "Ahmedabad", "Kochi", "Goa", "Jaipur"
    };

    for (const auto &src : cities) {            // nested loop = Cartesian product of cities
        for (const auto &dst : cities) {
            if (src == dst) continue;          // a city can't fly to itself

            std::pair<std::string, std::string> route = {src, dst};
            for (int i = 1; i < 10; ++i) {     // 9 flights numbered 1..9 per route
                // Flight ID = first 3 letters of each city + index, e.g. "HydAhm1".
                std::string code = src.substr(0, 3) + dst.substr(0, 3) + std::to_string(i);
                std::string time = generateRandomTime(TimeSlot::Morning);
                routes[route].emplace_back(code, time);   // map auto-creates the vector on first access
            }
        }
    }
}

// Reverse lookup: given a route and a departure time, return the matching flight ID.
// Linear scan over that route's options; "UNAVAILABLE" is the not-found sentinel.
std::string RouteManager::getNextFlightID(const std::string &src, const std::string &dst, const std::string &time) {
    std::pair<std::string, std::string> key = {src, dst};
    auto &options = routes[key];   // `auto&` = reference (no copy); reads from the live catalogue
    for (const auto &opt : options) {
        if (opt.second == time) return opt.first;   // opt = (flightID, time); .first is the ID
    }
    return "UNAVAILABLE";
}

// Return this route's flights sorted by departure time (earliest first).
// Returns a COPY (by value) so sorting doesn't disturb the stored catalogue.
// NOTE: the `slot` parameter is currently unused — all flights were generated in the
// Morning window — so it's reserved for future per-slot filtering.
std::vector<std::pair<std::string, std::string>> RouteManager::getFlightOptionsByTime(
    const std::string &src,
    const std::string &dst,
    TimeSlot slot
) {
    std::pair<std::string, std::string> key = {src, dst};
    auto options = routes[key];   // `auto` (no &) => copy; also note operator[] inserts an
                                  // empty entry if the route is missing (use .find to avoid that)

    // std::sort with a custom comparator lambda. a.second is the "HH:MM" string;
    // lexicographic string compare works as chronological order for zero-padded times.
    std::sort(options.begin(), options.end(), [](const auto &a, const auto &b) {
        return a.second < b.second; // sort by time
    });

    return options;
}
