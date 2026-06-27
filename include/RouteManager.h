#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <utility>
#include <random>
#include <algorithm>

// RouteManager = a "data generator" / mini route catalogue. It synthesizes a fixed
// set of flights between known cities at construction time, so the rest of the app has
// realistic flight options to book against without any external database.
class RouteManager {
public:
    // `enum class` (scoped enum, C++11): values are accessed as TimeSlot::Morning and
    // do NOT implicitly convert to int — type-safer than a plain C `enum`.
    enum class TimeSlot { Morning, Afternoon, Evening };

    RouteManager();   // ctor calls generateRoutes() to populate the catalogue eagerly

    // `static` = belongs to the class, not an instance; callable as RouteManager::normalize(..)
    // without an object. Used as a pure string utility (Title-cases a city name).
    static std::string normalize(const std::string &city);
    std::string getNextFlightID(const std::string &src, const std::string &dst, const std::string &time);
    std::vector<std::pair<std::string, std::string>> getFlightOptionsByTime(
        const std::string &src,
        const std::string &dst,
        TimeSlot slot
    );

private:
    // Key is a (source, destination) PAIR. std::map can key on a pair because std::pair
    // has a built-in lexicographic operator< — no custom comparator needed. Value is the
    // list of (flightID, time) options for that route.
    std::map<std::pair<std::string, std::string>, std::vector<std::pair<std::string, std::string>>> routes;
    std::map<std::string, int> flightCodeCounter;   // (reserved for sequential ID numbering)

    void generateRoutes();                                  // builds the full city x city catalogue
    std::string generateRandomTime(RouteManager::TimeSlot slot);  // random HH:MM within a slot's window
};

#endif
