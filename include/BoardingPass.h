// BoardingPass.h
#pragma once   // modern alternative to #ifndef include guards (compiler-supported, less boilerplate)
#include <string>
#include "Passenger.h"
#include "Flight.h"

// BoardingPass = pure output/formatting utility (single static factory method).
// Takes the data it needs (passenger + flight + seat) and produces a printed/saved
// ticket — it owns no state. Keeps presentation concerns out of BookingSystem.
class BoardingPass {
public:
    static void generate(const Passenger& p, const Flight& f, int row, int col);
};
