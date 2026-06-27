#ifndef FLIGHT_H
#define FLIGHT_H

#include <iostream>
#include <vector>
#include <string>

// Flight = the core domain entity. Models one scheduled flight + its seat inventory.
// Design choice: members are public (a "transparent value object"), so BookingSystem
// reads f.flightID directly. Trade-off vs strict encapsulation: simpler code, but no
// invariant protection — an interviewer may ask "why not make these private?".
class Flight {
public:
    // Flight details
    std::string flightID;
    std::string source, destination;
    std::string date, time;

    int rows, cols;        // seat grid dimensions (here 40 x 6)
    int totalSeats;        // = rows * cols, cached so we don't recompute
    int bookedSeats;       // running count; drives dynamic pricing & occupancy %
    int baseFare;

    // 2D grid of booked/free flags. vector<vector<bool>> is the intuitive "seat map".
    // Aside: vector<bool> is a SPACE-OPTIMISED bit-packed specialization (not a real
    // container of bool) — a classic C++ gotcha worth mentioning in interviews.
    std::vector<std::vector<bool>> seatMatrix;

    // Constructor: initializes via member-initializer list (see .cpp) and allocates
    // the seatMatrix to rows x cols, all false (every seat starts available).
    Flight(std::string id, std::string src, std::string dst,
           std::string dt, std::string tm, int r, int c, int fare);

    // Seat operations — all O(1) since they index directly into the grid.
    void displaySeats() const;                      // renders the ASCII seat map
    bool isSeatAvailable(int row, int col) const;   // also does bounds-checking
    void bookSeat(int row, int col);

    // Admin statistics support — read-only getters used by the Admin panel reports.
    int getTotalRows() const;
    int getTotalCols() const;
    int getBookedSeats() const;
    int getTotalSeats() const;
    int getRevenue() const;
    double getOccupancyRate() const;
};

#endif
