#pragma once

#include <cstdint>  // uint64_tのために必要

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000
#define BUFFER_SIZE 1024
#define SAMPLE_RATE 48000

// 値はMIDIイベントの仕様に準じる
typedef struct daw_event {
    int type;
    uint64_t event_time;
    int sample_offset;
    int channel;
    int key;
    int velocity;
    int event_type;
    int param_id;
    double param_value;
    bool erace_flag = false;
    bool is_note_on;
    bool is_note_off;
    double length;
    double delay_time;
    double stream_time;
    int note_id;
} daw_event_t;

enum {
    DAW_EVENT_NOTE_ON,
    DAW_EVENT_NOTE_OFF,
    DAW_EVENT_PARAM_CHANGE,
};

enum MidiStatus
{
    MIDI_STATUS_NOTE_OFF = 0x8,
    MIDI_STATUS_NOTE_ON = 0x9,
    MIDI_STATUS_NOTE_AT = 0xA, // after touch
    MIDI_STATUS_CC = 0xB,      // control change
    MIDI_STATUS_PGM_CHANGE = 0xC,
    MIDI_STATUS_CHANNEL_AT = 0xD, // after touch
    MIDI_STATUS_PITCH_BEND = 0xE,
};
