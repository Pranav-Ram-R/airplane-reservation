#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <string>

// LoginManager = stateless authentication gate. Both methods are static (no instance
// needed) — it's an "all static utility class". Separating auth from the rest is the
// Single Responsibility Principle in action.
class LoginManager {
public:
    static bool loginAsAdmin();                       // checks input against admin_credentials.txt
    // Takes aadhaar by NON-const reference (std::string&) = an OUT-parameter: on success
    // the caller's string is filled with the validated Aadhaar to use as the session key.
    static bool loginAsPassenger(std::string& aadhaar);
};

bool isValidAadhaar(const std::string& aadhaar);  // free helper: 12-digits-all-numeric check

#endif
