#include "../include/BoardingPass.h"
#include "../include/VoiceAssistant.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
// NOTE: the include block below is an accidental duplicate of the one above. Harmless
// because every header here has an include guard / #pragma once, but it's dead clutter
// that could be removed.

#include "../include/BoardingPass.h"
#include "../include/VoiceAssistant.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

// Generate a boarding pass: builds the formatted text once, then both WRITES it to a
// per-passenger file AND prints it to the console (single source of truth = `out`).
void BoardingPass::generate(const Passenger &p, const Flight &f, int row, int col) {
    // Convert 0-based (row,col) back to human seat label: row+1 and 'A'+col, e.g. 12C.
    std::string seat = std::to_string(row + 1) + static_cast<char>('A' + col);
    // Unique filename per (passenger, flight) so passes don't overwrite each other.
    std::string filename = "BoardingPass_" + p.passportNumber + "_" + f.flightID + ".txt";

    // Build the whole ticket in an in-memory string stream first (compose once, emit twice).
    std::ostringstream out;
    out << "====================================================\n";
    out << "                 DOMESTIC AIRLINES                  \n";
    out << "====================================================\n";
    out << "Passenger Name     : " << p.name << "\n";
    out << "Aadhaar Number     : " << p.passportNumber << "\n";
    out << "Flight ID          : " << f.flightID << "\n";
    out << "From               : " << f.source << "\n";
    out << "To                 : " << f.destination << "\n";
    out << "Date               : " << f.date << "\n";
    out << "Time               : " << f.time << "\n";
    out << "Seat               : " << seat << "\n";
    out << "Gate               : 5B\n";
    out << "Boarding Time      : 45 minutes before departure\n";
    out << "====================================================\n";
    out << "Note: Please carry a valid ID proof. Be at the gate\n";
    out << "at least 45 minutes before departure.\n";
    out << "====================================================\n";

    // Write to file. `if (!fileOut)` uses ofstream's bool conversion to test open failure.
    std::ofstream fileOut(filename);
    if (!fileOut) {
        std::string errorMsg = "Error generating boarding pass.";
        std::cout << errorMsg << std::endl;
        speak(errorMsg);
        return;
    }
    fileOut << out.str();   // out.str() materializes the accumulated text
    fileOut.close();

    // Print the same content on the console.
    std::cout << out.str();

    speak("Your boarding pass has been successfully generated.");
}
