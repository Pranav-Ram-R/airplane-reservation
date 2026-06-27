#include "../include/VoiceAssistant.h"
#include <cstdlib>
#include <string>

// Text-to-speech via the OS's native tool. Compile-time platform dispatch with #ifdef:
// only ONE branch is compiled, chosen by predefined macros (_WIN32 / __APPLE__ / else).
// This is the classic C/C++ way to write portable code that shells out to OS services.
void speak(const std::string &text) {
#ifdef _WIN32
    // Windows: drive the .NET System.Speech synthesizer from PowerShell.
    // We strip embedded double-quotes first because they'd otherwise break the nested
    // PowerShell command string (a small but real shell-injection / quoting hazard).
    std::string safeText = text;
    for (char &c : safeText) {
        if (c == '"') c = ' ';  // remove double-quotes to avoid PowerShell parsing issues
    }

    std::string command = "PowerShell -Command \"Add-Type -AssemblyName System.Speech; "
                          "$speak = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
                          "$speak.Rate = -2; $speak.Speak(\\\"" + safeText + "\\\")\"";
#elif __APPLE__
    std::string command = "say \"" + text + "\"";        // macOS built-in `say`
#else
    std::string command = "espeak -s 130 \"" + text + "\"";  // Linux: `espeak` (must be installed)
#endif
    // system() hands the assembled command to the OS shell. Blocking call: speech plays
    // synchronously, so the program pauses until the utterance finishes.
    system(command.c_str());
}
