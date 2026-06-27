#ifndef ADMIN_H
#define ADMIN_H

#include "BookingSystem.h"
#include <string>

// Admin = the administrative "controller". It never owns data; every method takes a
// BookingSystem& and operates on that shared state. showMenu() is the only entry point
// actually used; the other three are declared-but-unused planned breakdowns of it.
class Admin {
public:
    // Non-const BookingSystem& because showMenu() seeds flights into the system.
    void showMenu(BookingSystem &system);

    // const BookingSystem& = read-only views (reports that don't mutate state).
    void viewAllBookings(const BookingSystem &system);
    void viewFlightStats(const BookingSystem &system);
    void viewOccupancyAndRevenue(const BookingSystem &system);
};

#endif
