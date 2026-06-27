#include "../include/Pricing.h"
#include "../include/VoiceAssistant.h"
#include <string>

// Base-fare lookup table implemented as cascading if/else "fare bands".
// The doubled-up conditions (A->B and B->A) make the fare symmetric/bidirectional.
// IMPORTANT (interview gotcha): these compare LOWERCASE city names, but the rest of the
// app normalizes cities to Title-case ("Mumbai") before calling here — so in practice
// none of these branches hit and everything returns the ₹4000 default. Either lowercase
// the inputs here or change the literals to Title-case to "activate" real pricing.
int getBaseFare(const std::string &src, const std::string &dst) {
    if ((src == "mumbai" && dst == "pune") || (src == "pune" && dst == "mumbai") ||
        (src == "chennai" && dst == "bengaluru") || (src == "bengaluru" && dst == "chennai")) {
        return 3000;
    }

    if ((src == "mumbai" && dst == "goa") || (src == "goa" && dst == "mumbai") ||
        (src == "ahmedabad" && dst == "goa") || (src == "goa" && dst == "ahmedabad") ||
        (src == "ahmedabad" && dst == "mumbai") || (src == "mumbai" && dst == "ahmedabad")) {
        return 4500;
    }

    if ((src == "ahmedabad" && dst == "bengaluru") || (src == "bengaluru" && dst == "ahmedabad") ||
        (src == "ahmedabad" && dst == "chennai") || (src == "chennai" && dst == "ahmedabad") ||
        (src == "delhi" && dst == "chennai") || (src == "chennai" && dst == "delhi") ||
        (src == "delhi" && dst == "bengaluru") || (src == "bengaluru" && dst == "delhi") ||
        (src == "delhi" && dst == "kolkata") || (src == "kolkata" && dst == "delhi") ||
        (src == "hyderabad" && dst == "ahmedabad") || (src == "ahmedabad" && dst == "hyderabad")) {
        return 6000;
    }

    if ((src == "delhi" && dst == "kochi") || (src == "kochi" && dst == "delhi") ||
        (src == "kolkata" && dst == "kochi") || (src == "kochi" && dst == "kolkata") ||
        (src == "kochi" && dst == "jaipur") || (src == "jaipur" && dst == "kochi") ||
        (src == "kochi" && dst == "lucknow") || (src == "lucknow" && dst == "kochi")) {
        return 7500;
    }

    return 4000; // Default fare
}

// Dynamic / surge pricing: the fuller the flight, the higher the multiplier — mimicking
// real airline yield management. Pure function of (baseFare, occupancy).
double Pricing::calculateFare(int baseFare, int totalSeats, int bookedSeats) {
    // static_cast<double> on the numerator forces floating-point division; without it,
    // int/int would truncate the ratio to 0 and every flight would price at < 0.3.
    double occupancyRate = static_cast<double>(bookedSeats) / totalSeats;

    if (occupancyRate < 0.3) {            // up to 30% full: base price
        return baseFare;
    } else if (occupancyRate < 0.6) {     // 30-60%: +25%
        return baseFare * 1.25;
    } else if (occupancyRate < 0.9) {     // 60-90%: +50%
        return baseFare * 1.5;
    } else {                              // 90%+: double (peak surge)
        return baseFare * 2.0;
    }
}
