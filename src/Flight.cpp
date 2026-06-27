#include "../include/Flight.h"
#include "../include/VoiceAssistant.h"
#include <iomanip>  // for setw

// Constructor.
// The `: flightID(id), ...` part is the MEMBER-INITIALIZER LIST — members are constructed
// directly from these values. Preferred over assigning inside the body because it avoids
// a default-construct-then-assign for each member (matters for non-trivial types like
// std::string). Order of init follows DECLARATION order in the header, not the list order.
Flight::Flight(std::string id, std::string src, std::string dst,
               std::string dt, std::string tm, int r, int c, int fare)
    : flightID(id), source(src), destination(dst), date(dt), time(tm),
      rows(r), cols(c), baseFare(fare) {
    totalSeats = rows * cols;   // derived value
    bookedSeats = 0;
    // Allocate a rows x cols grid, every cell = false (all seats available).
    // Outer vector of `rows` inner vectors, each `cols` long, value-initialized to false.
    seatMatrix = std::vector<std::vector<bool>>(rows, std::vector<bool>(cols, false));
}

// Display seat layout
void Flight::displaySeats() const {
    std::cout << "\nSeat Layout for Flight " << flightID << ":\n";

    // Print column headers (A, B, C, ...)
    std::cout << "    ";
    for (int j = 0; j < cols; ++j) {
        std::cout << " " << static_cast<char>('A' + j) << " ";
    }
    std::cout << "\n";

    // Print seat rows
    for (int i = 0; i < rows; ++i) {
        std::cout << std::setw(2) << i + 1 << "  ";  // Row numbers
        for (int j = 0; j < cols; ++j) {
            std::cout << "[" << (seatMatrix[i][j] ? 'X' : ' ') << "]";
        }
        std::cout << "\n";
    }

    std::cout << "Note: [X] = Booked, [ ] = Available\n";
}

// Check if a seat is available.
// Bounds-checks FIRST so the `!seatMatrix[row][col]` access can never go out of range
// (short-circuit evaluation of && stops before indexing if the indices are invalid).
// Uses 0-based indices internally; the UI passes (row-1, col-'A').
bool Flight::isSeatAvailable(int row, int col) const {
    return row >= 0 && row < rows &&
           col >= 0 && col < cols &&
           !seatMatrix[row][col];   // true only if not yet booked
}

// Book a seat. Guarded by isSeatAvailable so a double-book is a no-op rather than a
// silent overwrite — keeps bookedSeats consistent with the grid. O(1).
void Flight::bookSeat(int row, int col) {
    if (isSeatAvailable(row, col)) {
        seatMatrix[row][col] = true;
        bookedSeats++;
    }
}

// Optional helper functions for admin statistics:

int Flight::getTotalRows() const {
    return rows;
}

int Flight::getTotalCols() const {
    return cols;
}

int Flight::getBookedSeats() const {
    return bookedSeats;
}

int Flight::getTotalSeats() const {
    return totalSeats;
}

int Flight::getRevenue() const {
    return bookedSeats * baseFare;
}

// Occupancy as a percentage. Ternary guards against divide-by-zero (totalSeats == 0).
// `100.0 *` forces DOUBLE arithmetic — without the .0 this would be integer division
// and truncate to 0 for any partly-full flight (a classic C++ interview trap).
double Flight::getOccupancyRate() const {
    return totalSeats > 0 ? (100.0 * bookedSeats / totalSeats) : 0.0;
}
