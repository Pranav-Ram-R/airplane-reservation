#ifndef PRICING_H
#define PRICING_H
#include <string>
// Pricing = a stateless "policy" class. All logic is in static methods, so it's used
// purely as a namespace of pure functions (no Pricing object is ever created).
// Interview angle: this is the Strategy/Policy pattern in miniature — fare rules are
// isolated here, so changing pricing never touches booking logic.
class Pricing {
public:
    // Dynamic (surge) pricing: final fare scales with how full the flight already is.
    // `static` => call as Pricing::calculateFare(...). Pure function: output depends
    // only on inputs, no side effects (easy to unit-test).
    static double calculateFare(int baseFare, int totalSeats, int bookedSeats);
};
// Free function (not a member): maps a city pair to its base fare via a lookup table.
int getBaseFare(const std::string &src, const std::string &dst);
#endif
