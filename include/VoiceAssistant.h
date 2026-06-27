#ifndef VOICE_ASSISTANT_H
#define VOICE_ASSISTANT_H

#include <string>

// Cross-platform text-to-speech. Declared here, implemented per-OS in the .cpp via
// preprocessor #ifdefs. Interview point: this is a thin "abstraction over a platform
// service" — callers say speak("..."); they don't care which TTS backend runs.
void speak(const std::string &text);

#endif // VOICE_ASSISTANT_H
