#pragma once

#define MA_NO_DECODING
#define MA_NO_ENCODING

#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


#include "clap/all.h"

#include <miniaudio.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <shlobj.h>

#include "miniaudio.h"
#include "RtMidi.h"

#include "common.hpp"
#include "audio_plugin_host.hpp"


class DawEngine 
{

public:
    DawEngine();
    ~DawEngine();

    enum State
    {
        kStateStopped,
        kStateRunning,
        kStateStopping,
    };

    int init(std::string plugin_dir, std::string plugin_filename);
    int deinit();
    int init_audio();
    int deinit_audio();
    int init_midi();
    int deinit_midi();
    bool load_plugin(const char *path);

    bool start();
    void stop();

    int update(double delta);

    void set_plugin_directory(std::string plugin_dir);

    void process_audio(const float *input, float *output, uint32_t frame_count, double stream_time);

    void play_note(int key, double length, int velocity, int channel, double delay_time);
    int add_note_on(uint64_t event_time, int key, int velocity, int channel);
    int add_note_off(uint64_t event_time, int key, int velocity, int channel);
    int add_param_change(godot::String name, double value, int channel, double delay_time);
    int add_param_change_by_id(int param_id, double value, int channel, double delay_time);
    void extract_upcoming_events();
    std::string get_clap_plugin_info();

    ma_waveform wave_form;
    ma_waveform_config wave_form_config;
    std::atomic<bool> is_processing;
    uint64_t total_frames_processed = 0;

    Json::Value loaded_plugin_params_json;

    std::vector<float> daw_audio_input_buffer;
    std::vector<float> daw_audio_output_buffer;
    
   float *_inputs[2] = {nullptr, nullptr};
   float *_outputs[2] = {nullptr, nullptr};
   

private:
    AudioPluginHost audio_plugin_host;

    ma_context context;
    ma_device device;
    ma_context_config context_config;
    int device_open_result = -9999;
    RtMidiIn *midiin = 0;
    RtMidiOut *midiout = 0;
    int selected_audio_playback_device_index = 2;
    int selected_input_midi_port = 0;
    int selected_output_midi_port = 2;
    int device_sample_rate = DEVICE_SAMPLE_RATE;
    int device_channels = DEVICE_CHANNELS;
    int device_format = DEVICE_FORMAT;
    int min_frames_count = 32;
    int max_frames_count = 4096;
    const std::string default_clap_folder_path_str = "C:/Program Files/Common Files/CLAP/";
    std::string plugin_dir_path = "./";
    //std::string clap_file_name = "Odin2.clap";
	int note_id = 0;

    std::vector<daw_event_t> _daw_events;

    //std::string clap_file_name = "Dexed.clap";
    std::string clap_file_name = "clap-saw-demo-imgui.clap";
    //std::string clap_file_name = "Surge Synth Team/Surge XT.clap";// "Dexed.clap"; //"Odin2.clap";

    bool is_initialized = false;
    bool is_input_init_success = true;
    bool is_output_init_success = true;
    bool is_audio_init_success = false;
    bool is_midi_init_success = false;

    //const float* inputs[2];
    //float* outputs[2];

    float *audio_input;
    float *audio_output;

	clap::helpers::EventList _event_in;
	clap::helpers::EventList _event_out;

    // miniaudioのコールバック
    static void audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);

};
