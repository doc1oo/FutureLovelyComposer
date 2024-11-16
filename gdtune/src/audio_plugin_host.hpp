#pragma once

#include <iostream>
#include <fstream>
#include <windows.h>

#include "clap/all.h"

#include <clap-helpers/host.hh>
#include <clap-helpers/event-list.hh>
#include "clap-scanner.h"
#include "clap-info-host.h"
// #include "info.h"

#include "common.hpp"

/*
#include <clap/clap.h>
#include <clap-helpers/event-list.hh>
#include <clap-helpers/reducing-param-queue.hh>
*/

#include "json/json.h"

struct CLAPInfoJsonRoot
{
	Json::Value root;
	bool active{true};
	std::string outFile;

	~CLAPInfoJsonRoot()
	{
		std::cout << "CLAPInfoJsonRoot::~CLAPInfoJsonRoot() start" << std::endl;
		if (active)
		{
			Json::StyledWriter writer;
			std::string out_string = writer.write(root);
			if (outFile.empty())
			{
				std::cout << out_string << std::endl;
			}
			else
			{
				auto ofs = std::ofstream(outFile);
				if (ofs.is_open())
				{
					ofs << out_string;
					ofs.close();
				}
				else
				{
					std::cout << "Unable to open output file '" << outFile << "' for writing";
				}
			}
		}
		std::cout << "CLAPInfoJsonRoot::~CLAPInfoJsonRoot() end" << std::endl;
	}
};

class AudioPluginHost
{

public:
	AudioPluginHost();
	~AudioPluginHost();

	int init();
	int deinit();

	int init_clap_plugin(std::vector<std::filesystem::path> &clap_file_pathes);
	int deinit_clap_plugin();

	int device_sample_rate = DEVICE_SAMPLE_RATE;
	//int min_frames_count = 32;
	//int max_frames_count = 4096;
	int min_frames_count = BUFFER_SIZE;
	int max_frames_count = BUFFER_SIZE;

	clap_host_t *clap_host = nullptr;
	clap_plugin_factory_t *clap_plugin_factory = nullptr;
	const clap_plugin_entry_t *clap_plugin_entry = nullptr;
	const clap_plugin_t *clap_plugin = nullptr;
	const clap_plugin_descriptor_t *clap_plugin_descriptor = nullptr;

	int plugin_process(const float *audio_in, float *audio_out, int frame_count, double stream_time);
	int process_note_on(int sample_offset, daw_event_t &event);
	int process_note_off(int sample_offset, daw_event_t &event);
	int process_param_change(int sample_offset, daw_event_t &event);
	void set_ports(int num_inputs, float **inputs, int num_outputs, float **outputs);

	void plugin_activate(int32_t sample_rate, int32_t blockSize);

	CLAPInfoJsonRoot& get_clap_plugin_info();
	Json::Value get_loaded_plugin_params_json();
	Json::Value get_plugin_params_json(const clap_plugin_t *inst);
	int get_clap_info(std::vector<std::filesystem::path> &sp);

	/* process stuff */
    clap_audio_buffer_t input_clap_audio_buffer{};
    clap_audio_buffer_t output_clap_audio_buffer{};
	clap::helpers::EventList _event_in;
	clap::helpers::EventList _event_out;
	
    clap_process_t clap_process{};
    std::vector<float> daw_audio_input_buffer;
    std::vector<float> daw_audio_output_buffer;
    
    double g_phase = 0.0;
	int note_id = 0;
    CLAPInfoJsonRoot clap_info_doc;

    // ホストコールバック関数
    static void host_request_restart(const clap_host_t *);
    static void host_request_process(const clap_host_t *);
    static void host_request_callback(const clap_host_t *);
    static void host_log(const clap_host_t *, clap_log_severity severity, const char *msg);


private:
	HMODULE _handle_clap_plugin_module = nullptr;
};

/*
TCHAR waFolderPath[ MAX_PATH ];
SHGetSpecialFolderPath(NULL, waFolderPath, CSIDL_PROGRAM_FILES_COMMON, FALSE);
godot::UtilityFunctions::print( std::format("Windows Default CLAP Path: {}", waFolderPath ).c_str() );

Json::Value res;
auto sp = clap_scanner::validCLAPSearchPaths();
for (const auto &q : sp) {
	res.append(q.string());
	godot::UtilityFunctions::print( std::format("CLAP Search Path: {}", q.string() ).c_str() );
}
*/
// Json::Value sp;
// sp.append("C:/Program Files/Common Files/CLAP/");

/*
for (const auto &p : sp)
{
	try
	{
		for (auto const &dir_entry : std::filesystem::recursive_directory_iterator(p) )
		{
			if (dir_entry.path().extension().u8string() == u8".clap")
			{
				if (!std::filesystem::is_directory(dir_entry.path()))
				{
					claps.emplace_back(dir_entry.path());
					godot::UtilityFunctions::print( std::format("CLAP Search Path: {}", dir_entry.path().string() ).c_str() );
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error &)
	{
	}
}
*/
