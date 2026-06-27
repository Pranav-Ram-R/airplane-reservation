#ifndef PASSENGER_H
#define PASSENGER_H

#include <string>

// Passenger = lightweight value object copied freely into vectors / the priority_queue.
// Naming caveat (interview-honest): `passportNumber` actually stores the 12-digit
// AADHAAR number — the field name is a historical leftover, not a second ID.
class Passenger {
public:
    std::string name, passportNumber;   // passportNumber == Aadhaar (acts as unique user key)
    int age;                            // also the sort key for the waitlist max-heap
    char gender;                        // 'M' / 'F'

    Passenger(std::string n, std::string pno, int a, char g);
};

#endif
