#include "../include/Passenger.h"
#include "../include/VoiceAssistant.h"
// Constructor with member-initializer list and an empty body — it does no work beyond
// copying the four fields, so there's nothing inside the {}. (pno -> passportNumber,
// which actually holds the Aadhaar number; see Passenger.h.)
Passenger::Passenger(std::string n, std::string pno, int a, char g)
    : name(n), passportNumber(pno), age(a), gender(g) {}
