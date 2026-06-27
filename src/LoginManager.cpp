#include "../include/LoginManager.h"
#include "../include/VoiceAssistant.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Validates Aadhaar = exactly 12 characters AND every char is a digit.
// std::all_of (algorithm) returns true only if the predicate holds for ALL elements.
// `::isdigit` (leading :: = global-namespace C function) is passed as the predicate.
bool isValidAadhaar(const std::string& aadhaar) {
    return aadhaar.length() == 12 && std::all_of(aadhaar.begin(), aadhaar.end(), ::isdigit);
}

bool LoginManager::loginAsPassenger(std::string& aadhaar) {
    std::cout << "Enter Aadhaar Number (12-digit): ";
    std::cin >> aadhaar;
    if (!isValidAadhaar(aadhaar)) {
        std::cout << "Invalid Aadhaar number.\n";
        speak("Invalid Aadhaar number.");
        return false;
    }
    return true;
}

bool LoginManager::loginAsAdmin() {
    std::string inputUsername, inputPassword;

    std::cout << "Enter Admin Username: ";
    std::cin >> inputUsername;
    std::cout << "Enter Admin Password: ";
    std::cin >> inputPassword;

    std::ifstream file("admin_credentials.txt");
    if (!file.is_open()) {
        std::cout << "Could not open admin_credentials.txt\n";
        speak("Login failed.");
        return false;
    }

    // Linear scan of the credential file: split each "user,pass" line and compare.
    // SECURITY CAVEAT (good to acknowledge in interviews): passwords are stored and
    // compared in PLAINTEXT. A real system would store a salted hash and use a
    // constant-time comparison to resist timing attacks.
    std::string line;
    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string storedUsername, storedPassword;
        getline(ss, storedUsername, ',');   // up to the comma = username
        getline(ss, storedPassword);        // rest of line = password

        if (inputUsername == storedUsername && inputPassword == storedPassword) {
            return true;                    // match found -> authenticated
        }
    }

    std::cout << "Invalid username or password.\n";
    speak("Invalid admin credentials.");
    return false;
}
