#include "core/apu/MidiOut.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "RtMidi.h"

// GB channel → MIDI channel (0-indexed)
// All melodic channels share MIDI channel 0 for polyphony on a single instrument.
// Channel 3 (noise): percussion on MIDI channel 9 (GM standard).
static const int kMidiChannel[4] = {0, 0, 0, 9};

// Noise channel: map NR43 clock shift (bits 7:4) to a GM percussion note.
// Higher shift = lower frequency: kick at top, hi-hat at bottom.
static const int kNoiseNote[16] = {
    42, 42, 38, 38, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36
};

MidiOut::MidiOut() : active_notes_({-1, -1, -1, -1}) {
    try {
        midi_ = std::make_unique<RtMidiOut>();
        midi_->openVirtualPort("VBA-M MIDI Out");
    } catch (...) {
        midi_.reset();
    }
}

MidiOut::~MidiOut() {
    allNotesOff();
}

int MidiOut::freqToMidi(int gb_channel, int freq_reg) {
    if (freq_reg >= 2048)
        return 0;
    // Wave channel runs at half the rate of the square channels
    double freq_hz = (gb_channel == 2 ? 65536.0 : 131072.0) / (2048 - freq_reg);
    int note = static_cast<int>(std::round(69.0 + 12.0 * std::log2(freq_hz / 440.0)));
    return std::max(0, std::min(127, note));
}

int MidiOut::noiseToMidi(int nr43) {
    return kNoiseNote[(nr43 >> 4) & 0x0F];
}

void MidiOut::noteOn(int gb_channel, int freq_reg, int volume) {
    if (!midi_ || gb_channel == 3)
        return;

    int note = freqToMidi(gb_channel, freq_reg);

    // Drop notes below C3 — sub-bass frequencies don't translate well to MIDI.
    if (note < 48)
        return;

    // If the same note is already playing, don't retrigger — GB games constantly
    // retrigger notes to keep them alive, which would sound like rapid slamming.
    if (note == active_notes_[gb_channel])
        return;

    noteOff(gb_channel);

    int midi_ch = kMidiChannel[gb_channel];
    int velocity = std::max(1, (volume * 127) / 15);

    std::vector<uint8_t> msg = {
        static_cast<uint8_t>(0x90 | midi_ch),
        static_cast<uint8_t>(note),
        static_cast<uint8_t>(velocity)
    };
    midi_->sendMessage(&msg);
    active_notes_[gb_channel] = note;
}

void MidiOut::noteOff(int gb_channel) {
    if (!midi_ || active_notes_[gb_channel] < 0)
        return;

    int midi_ch = kMidiChannel[gb_channel];
    std::vector<uint8_t> msg = {
        static_cast<uint8_t>(0x80 | midi_ch),
        static_cast<uint8_t>(active_notes_[gb_channel]),
        0
    };
    midi_->sendMessage(&msg);
    active_notes_[gb_channel] = -1;
}

void MidiOut::allNotesOff() {
    for (int i = 0; i < 4; i++)
        noteOff(i);
}
