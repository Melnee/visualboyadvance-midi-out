#pragma once

#include <array>
#include <cstdint>
#include <memory>

class RtMidiOut;

class MidiOut {
public:
    MidiOut();
    ~MidiOut();

    void noteOn(int gb_channel, int freq_reg, int volume);
    void noteOff(int gb_channel);
    void allNotesOff();

private:
    static int freqToMidi(int gb_channel, int freq_reg);
    static int noiseToMidi(int nr43);

    std::unique_ptr<RtMidiOut> midi_;
    std::array<int, 4> active_notes_;
};
