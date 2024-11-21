
#define NOMINMAX // clap-helperのmin/maxエラー対策

#include <string>
#include <iostream>
#include <windows.h>
#include <shlobj_core.h>
#include <numbers>

#include <godot_cpp/variant/utility_functions.hpp>

#include "audio_plugin_host.hpp"

enum class ThreadType
{
    Unknown,
    MainThread,
    AudioThread,
    AudioThreadPool,
};

thread_local ThreadType g_thread_type = ThreadType::Unknown;

AudioPluginHost::AudioPluginHost()
{
    godot::UtilityFunctions::print("AudioPluginHost() start");
    spdlog::trace(__FUNCTION__);
    LOG_FUNC_START();

    g_thread_type = ThreadType::MainThread;
    // オーディオバッファの初期化
    /*
    inputBuffer.resize(BUFFER_SIZE * 2);  // ステレオ
    outputBuffer.resize(BUFFER_SIZE * 2);
    inputs[0] = &inputBuffer[0];
    inputs[1] = &inputBuffer[BUFFER_SIZE];
    outputs[0] = &outputBuffer[0];
    outputs[1] = &outputBuffer[BUFFER_SIZE];
    */
    godot::UtilityFunctions::print("AudioPluginHost() end");
    spdlog::trace("{} end", __FUNCTION__);
}

AudioPluginHost::~AudioPluginHost()
{
    godot::UtilityFunctions::print("AudioPluginHost::~AudioPluginHost start");
      spdlog::trace(__FUNCTION__);
      spdlog::trace("{} end", __FUNCTION__);
    godot::UtilityFunctions::print("AudioPluginHost::~AudioPluginHost end");
}

int AudioPluginHost::init()
{
    godot::UtilityFunctions::print("AudioPluginHost::init() start");
    // init_clap_plugin();
    godot::UtilityFunctions::print("AudioPluginHost::init() end");

    return 0;
}

int AudioPluginHost::init_clap_plugin(std::vector<std::filesystem::path> &clap_file_pathes)
{
    godot::UtilityFunctions::print(std::format("init_clap_plugin() start").c_str());

    Json::Value &root = clap_info_doc.root;

    Json::Value entryJson;
    auto clap_path = clap_file_pathes[0];

    auto clap_file_path_string = clap_path.string();

    bool annExt{false};
    bool paramShow{true};
    bool audioPorts{true};
    bool notePorts{true};
    bool otherExt{true};
    bool searchPath{false};
    bool brief{false};

    Json::Value res;
    /*
    for (const auto &q : clap_file_pathes)
    {
        Json::Value entryJson;
        if (auto entry = clap_scanner::entryFromCLAPPath(q))
        {
            entry->init(q.string().c_str());
            entryJson["path"] = q.string();

            //entryJson["clap-version"] = std::to_string(entry->clap_version.major) + "." +
            //                            std::to_string(entry->clap_version.minor) + "." +
            //                            std::to_string(entry->clap_version.revision);

            // entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
        }
    }*/

    // clap_plugin_entry = clap_scanner::entryFromCLAPPath(clap_path);

    auto h_module = LoadLibrary((LPCSTR)(clap_path.generic_string().c_str()));
    if (!h_module)
        return -1;

    _handle_clap_plugin_module = h_module;

    auto p_mod = GetProcAddress(_handle_clap_plugin_module, "clap_entry");
    //    std::cout << "phan is " << phan << std::endl;
    auto entry = (clap_plugin_entry_t *)p_mod;

    if (entry)
    {
        clap_plugin_entry = entry;

        // clap_plugin_entry_tに対してのinit
        // 実際のプラグインのインスタンスの生成は get_factory(CLAP_PLUGIN_FACTORY_ID);を呼んでclap_plugin_factory_tを取得してから
        entry->init(clap_path.string().c_str()); // q.string()はclapファイルのパス

        // エラー対策
        if (!entry)
        {
            std::cerr << "   clap_entry returned a nullptr\n"
                      << "   either this plugin is not a CLAP or it has exported the incorrect symbol."
                      << std::endl;
            clap_info_doc.active = false;
            return 3;
        }

        Json::Value &root = clap_info_doc.root;
        
        root["file"] = clap_path.string();

        auto version = entry->clap_version;
        std::stringstream ss;
        ss << version.major << "." << version.minor << "." << version.revision;
        root["clap-version"] = ss.str();
        
        // プラグインファクトリーの取得
        clap_plugin_factory = (clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
        auto &fact = clap_plugin_factory; // よく使うので名前短縮のため
        auto plugin_count = fact->get_plugin_count(fact);
        if (plugin_count <= 0)
        {
            std::cerr << "Plugin factory has no plugins" << std::endl;
            clap_info_doc.active = false;
            return 4;
        }

        int select_plugin_index = 0;

        clap_plugin_descriptor = fact->get_plugin_descriptor(fact, select_plugin_index);
        auto &desc = clap_plugin_descriptor;

        auto &doc_desc = root["plugin_descriptor"];

        doc_desc["id"] = desc->id;
        doc_desc["name"] = desc->name;
        doc_desc["vendor"] = desc->vendor;
        doc_desc["version"] = desc->version;
        doc_desc["description"] = desc->description;
        doc_desc["manual_url"] = desc->manual_url;
        doc_desc["support_url"] = desc->support_url;
        doc_desc["url"] = desc->url;
        doc_desc["features"] = desc->features;
        
        auto desc_clap_ver = desc->clap_version;
        ss.str("");
        ss.clear();
        ss << desc_clap_ver.major << "." << desc_clap_ver.minor << "." << desc_clap_ver.revision;
        root["descriptor"]["clap_version"] = ss.str();

        // Now lets make an instance
        clap_host = clap_info_host::createCLAPInfoHost();
        auto &host = clap_host;

        host->host_data = this;
        host->clap_version = CLAP_VERSION;
        host->name = "Minimal CLAP Host";
        host->version = "1.0.0";
        host->vendor = "Example";
        host->vendor = "clap";
        host->url = "https://github.com/free-audio/clap";
        // host->get_extension = AudioPluginHost::clapExtension;
        host->request_restart = host_request_restart;
        host->request_process = host_request_process;
        host->request_callback = host_request_callback;

        clap_plugin = fact->create_plugin(fact, host, desc->id);
        auto &plugin = clap_plugin;
        if (!plugin)
        {
            std::cerr << "Unable to create plugin; inst is null" << std::endl;
            clap_info_doc.active = false;
            return 5;
        }

        // 実際のプラグインの初期化？
        bool result = plugin->init(plugin);
        if (!result)
        {
            std::cerr << "Unable to init plugin" << std::endl;
            clap_info_doc.active = false;
            return 6;
        }

        
        /*
        plugin->get_extension(plugin, CLAP_EXT_PARAMS, &clap_host_extension);
        if (!clap_host_extension)
        {
            std::cerr << "Unable to get extension" << std::endl;
            clap_info_doc.active = false;
            return 7;
        }
        */
                

        
        // 2. scan the params.
            /*
        auto count = plugin->paramsCount();
        std::unordered_set<clap_id> paramIds(count * 2);

        for (int32_t i = 0; i < count; ++i) {
            clap_param_info info;
            if (!_plugin->paramsGetInfo(i, &info))
                throw std::logic_error("clap_plugin_params.get_info did return false!");

            if (info.id == CLAP_INVALID_ID) {
                std::ostringstream msg;
                msg << "clap_plugin_params.get_info() reported a parameter with id = CLAP_INVALID_ID"
                    << std::endl
                    << " 2. name: " << info.name << ", module: " << info.module << std::endl;
                throw std::logic_error(msg.str());
            }
            auto it = _params.find(info.id);

            // check that the parameter is not declared twice
            if (paramIds.count(info.id) > 0) {
                Q_ASSERT(it != _params.end());

                std::ostringstream msg;
                msg << "the parameter with id: " << info.id << " was declared twice." << std::endl
                    << " 1. name: " << it->second->info().name << ", module: " << it->second->info().module
                    << std::endl
                    << " 2. name: " << info.name << ", module: " << info.module << std::endl;
                throw std::logic_error(msg.str());
            }
            paramIds.insert(info.id);

            if (it == _params.end()) {
                if (!(flags & CLAP_PARAM_RESCAN_ALL)) {
                    std::ostringstream msg;
                    msg << "a new parameter was declared, but the flag CLAP_PARAM_RESCAN_ALL was not "
                        "specified; id: "
                        << info.id << ", name: " << info.name << ", module: " << info.module << std::endl;
                    throw std::logic_error(msg.str());
                }

                double value = getParamValue(info);
                auto param = std::make_unique<PluginParam>(*this, info, value);
                checkValidParamValue(*param, value);
                _params.insert_or_assign(info.id, std::move(param));
            } else {
                // update param info
                if (!it->second->isInfoEqualTo(info)) {
                    if (!clapParamsRescanMayInfoChange(flags)) {
                    std::ostringstream msg;
                    msg << "a parameter's info did change, but the flag CLAP_PARAM_RESCAN_INFO "
                            "was not specified; id: "
                        << info.id << ", name: " << info.name << ", module: " << info.module
                        << std::endl;
                    throw std::logic_error(msg.str());
                    }

                    if (!(flags & CLAP_PARAM_RESCAN_ALL) &&
                        !it->second->isInfoCriticallyDifferentTo(info)) {
                    std::ostringstream msg;
                    msg << "a parameter's info has critical changes, but the flag CLAP_PARAM_RESCAN_ALL "
                            "was not specified; id: "
                        << info.id << ", name: " << info.name << ", module: " << info.module
                        << std::endl;
                    throw std::logic_error(msg.str());
                    }

                    it->second->setInfo(info);
                }

                double value = getParamValue(info);
                if (it->second->value() != value) {
                    if (!clapParamsRescanMayValueChange(flags)) {
                    std::ostringstream msg;
                    msg << "a parameter's value did change but, but the flag CLAP_PARAM_RESCAN_VALUES "
                            "was not specified; id: "
                        << info.id << ", name: " << info.name << ", module: " << info.module
                        << std::endl;
                    throw std::logic_error(msg.str());
                    }

                    // update param value
                    checkValidParamValue(*it->second, value);
                    it->second->setValue(value);
                    it->second->setModulation(value);
                }
            }
        }
            */
    }

    godot::UtilityFunctions::print(std::format("init_clap_plugin() end").c_str());
}

Json::Value AudioPluginHost::get_loaded_plugin_params_json()
{
    if (false) {
    }

    return get_plugin_params_json(clap_plugin);
}

Json::Value AudioPluginHost::get_plugin_params_json(const clap_plugin_t *inst)
{
    //std::cout << json["param-info"].toStyledString() << std::endl;


    auto inst_param = (clap_plugin_params_t *)inst->get_extension(inst, CLAP_EXT_PARAMS);

    Json::Value pluginParams;
    if (inst_param)
    {
        auto pc = inst_param->count(inst);
        pluginParams["implemented"] = true;
        pluginParams["param-count"] = pc;

        Json::Value instParams;
        instParams.resize(0);
        for (auto i = 0U; i < pc; ++i)
        {
            clap_param_info_t info;
            inst_param->get_info(inst, i, &info);

            Json::Value instParam;

            //std::stringstream ss;
            //ss << "0x" << std::hex << inf.id;
            //instParam["id"] = ss.str();
            instParam["id"] = info.id;

            instParam["module"] = info.module;
            instParam["name"] = info.name;

            Json::Value values;
            values["min"] = info.min_value;
            values["max"] = info.max_value;
            values["default"] = info.default_value;
            double d;
            inst_param->get_value(inst, info.id, &d);
            values["current"] = d;
            instParam["values"] = values;

            auto cp = [&info, &instParam](auto x, auto y) {
                if (info.flags & x)
                {
                    instParam["flags"].append(y);
                }
            };
            cp(CLAP_PARAM_IS_STEPPED, "stepped");
            cp(CLAP_PARAM_IS_PERIODIC, "periodic");
            cp(CLAP_PARAM_IS_HIDDEN, "hidden");
            cp(CLAP_PARAM_IS_READONLY, "readonly");
            cp(CLAP_PARAM_IS_BYPASS, "bypass");

            cp(CLAP_PARAM_IS_AUTOMATABLE, "auto");
            cp(CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID, "auto-noteid");
            cp(CLAP_PARAM_IS_AUTOMATABLE_PER_KEY, "auto-key");
            cp(CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL, "auto-channel");
            cp(CLAP_PARAM_IS_AUTOMATABLE_PER_PORT, "auto-port");

            cp(CLAP_PARAM_IS_MODULATABLE, "mod");
            cp(CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID, "mod-noteid");
            cp(CLAP_PARAM_IS_MODULATABLE_PER_KEY, "mod-key");
            cp(CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL, "mod-channel");
            cp(CLAP_PARAM_IS_MODULATABLE_PER_PORT, "mod-port");

            cp(CLAP_PARAM_REQUIRES_PROCESS, "requires-process");
            instParams.append(instParam);
        }
        pluginParams["param-info"] = instParams;
    }
    else
    {
        pluginParams["implemented"] = false;
    }
    return pluginParams;
}

CLAPInfoJsonRoot& AudioPluginHost::get_clap_plugin_info()
{
    spdlog::trace(__FUNCTION__);
    godot::UtilityFunctions::print("get_clap_plugin_info() start");

    return clap_info_doc;
}

void AudioPluginHost::plugin_activate(int32_t sample_rate, int32_t blockSize)
{
    spdlog::trace(__FUNCTION__);

    if (!clap_plugin)
        return;

    auto &plugin = clap_plugin;

    // プラグインのアクティベーション
    plugin->activate(plugin, device_sample_rate, min_frames_count, max_frames_count);

    bool result = plugin->start_processing(plugin);
    if (!result)
    {
        godot::UtilityFunctions::print(std::format("Unable to start_processing plugin").c_str());
        return;
    }
}

int AudioPluginHost::deinit()
{
    godot::UtilityFunctions::print("AudioPluginHost::deinit() start");
    spdlog::trace(__FUNCTION__);

    deinit_clap_plugin();

    godot::UtilityFunctions::print("AudioPluginHost::deinit() end");

    return 0;
}

int AudioPluginHost::deinit_clap_plugin()
{
    godot::UtilityFunctions::print("AudioPluginHost::deinit_clap_plugin() start");

    auto &plugin = clap_plugin;
    if (plugin)
    {
        plugin->stop_processing(plugin);
        plugin->deactivate(plugin);
        plugin->destroy(plugin);
    }

    if (clap_plugin_entry)
    {
        clap_plugin_entry->deinit();
    }

    // DLL を解放
    if (_handle_clap_plugin_module)
    {
        if (FreeLibrary(_handle_clap_plugin_module))
        {
            std::cout << "DLL successfully unloaded" << std::endl;
        }
        else
        {
            std::cerr << "Failed to unload DLL" << std::endl;
        }
    }

    godot::UtilityFunctions::print("AudioPluginHost::deinit_clap_plugin() end");

    return 0;
}

int AudioPluginHost::plugin_process(const float *audio_in, float *audio_out, int frame_count, double stream_time)
{
    spdlog::trace(__FUNCTION__);
    // godot::UtilityFunctions::print("AudioPluginHost::plugin_process() start");

    // godot::UtilityFunctions::print(std::format("buffer_size:  {}, {}", frame_count, BUFFER_SIZE).c_str());

    /*
    godot::UtilityFunctions::print(std::format("test1").c_str());
    float l_in[BUFFER_SIZE];
    float r_in[BUFFER_SIZE];

    float *inputs[2] = {l_in, r_in};

    godot::UtilityFunctions::print(std::format("test11").c_str());
    in_buffer.data32 = inputs;
    godot::UtilityFunctions::print(std::format("test12").c_str());

    godot::UtilityFunctions::print(std::format("test2").c_str());

    float l_out[BUFFER_SIZE];
    float r_out[BUFFER_SIZE];
    float *outputs[2] = {l_out, r_out};

    godot::UtilityFunctions::print(std::format("test3").c_str());
    */

    /*
     std::vector<float> inputBuffer[BUFFER_SIZE*2];
     std::vector<float> outputBuffer[BUFFER_SIZE*2];


     // プレーナー形式の入力/出力バッファ
     std::vector<float>& leftIn = inputBuffer;              // 0 ～ BUFFER_SIZE-1
     std::vector<float>& rightIn = inputBuffer[BUFFER_SIZE];// BUFFER_SIZE ～ (BUFFER_SIZE*2)-1

     std::vector<float>& leftOut = outputBuffer;            // 同上
     std::vector<float>& rightOut = outputBuffer[BUFFER_SIZE];
     */

    // in_buffer.data32 = t1;//const_cast<float **>(&audio_in);
    // out_buffer.data32 = t2;//const_cast<float **>(&audio_out);
    // in_buffer.channel_count = 2;
    // out_buffer.channel_count = 2;

    // in_buffer.data32 = const_cast<float**>(audio_in);
    // in_buffer.channel_count = 2;
    // out_buffer.data32 = audio_out;
    // out_buffer.channel_count = 2;

    /*
    godot::UtilityFunctions::print(std::format("test").c_str());
    process.audio_inputs = &in_buffer;
    process.audio_inputs_count = 1;
    process.audio_outputs = &out_buffer;
    process.audio_outputs_count = 1;
    process.frames_count = BUFFER_SIZE;
    */

    auto &process = clap_process;
    process.audio_inputs = &input_clap_audio_buffer;
    process.audio_inputs_count = 1;
    process.audio_outputs = &output_clap_audio_buffer;
    process.audio_outputs_count = 1;

    process.steady_time = -1; // -1は利用不可の場合　前フレームとの差分時間　次のプロセス呼び出しのために少なくとも `frames_count` だけ増加する必要があります。
    process.frames_count = frame_count;

    process.transport = nullptr;

    process.in_events = _event_in.clapInputEvents();
    process.out_events = _event_out.clapOutputEvents();

    auto ev_len = _event_in.size();
    // godot::UtilityFunctions::print(std::format).c_str());

    //_event_out.clear();
    // generatePluginInputEvents();

    // godot::UtilityFunctions::print(std::format("clap_plugin->process() start / {}, {}", frame_count, BUFFER_SIZE).c_str());

    // clap_plugin->process(clap_plugin, &process);        // 実際のCALPプラグインに処理させる。渡すデータ下手するとアプリごと落ちる

    if (clap_plugin != nullptr)
    {
        clap_plugin->process(clap_plugin, &process);
    }
    else
    {
        godot::UtilityFunctions::print("clap_plugin is NULL");
    }

    /* 
    for (int i = 0; i < frame_count; i++)
    {
        // ma_int16* output = (ma_int16*)pOutput;
        //float *output = output_clap_audio_buffer.data32[0];
        float sample = 0.9 * sinf(g_phase);

        output_clap_audio_buffer.data32[0][i] = sample; // 左チャンネル
        output_clap_audio_buffer.data32[0][i+BUFFER_SIZE] = sample; // 右チャンネル
        //output_clap_audio_buffer.data32[0][i + BUFFER_SIZE] = sample; // 右チャンネル
        //output_clap_audio_buffer.data32[1][i + BUFFER_SIZE] = sample; // 右チャンネル

        // フェーズを周波数とサンプルレートに基づいて進める
        g_phase += 0.1; // 2.0f * std::numbers::pi * g_frequency / g_sampleRate;

        // フェーズを2πでリセット
        if (g_phase >= 2.0f * std::numbers::pi)
        {
            g_phase -= 2.0f * std::numbers::pi;
        }
    } */

    if (ev_len >= 0)
    {
        std::cout << "_event_in.size(): " << _event_in.size() << std::endl;
/*
        float ar[30];
         for (int i = 0; i < 30; i++)
        {
            // std::cout << "" << output_clap_audio_buffer.data32[0][i];
            ar[i] = output_clap_audio_buffer.data32[0][i];

            printf("%f,", ar[i]);
            /*
            auto ev = _event_in.pop();
            if (ev->type == CLAP_EVENT_NOTE_ON) {
                auto note = (clap_event_note *)ev;
                std::cout << "Note On: " << note->key << std::endl;
            }
            else if (ev->type == CLAP_EVENT_NOTE_OFF) {
                auto note = (clap_event_note *)ev;
                std::cout << "Note Off: " << note->key << std::endl;
            }
        }

        printf("\n");
        // std::cout << "ar:{}" <<  ar[0] << std::endl; */
    }
    // godot::UtilityFunctions::print("clap_plugin->process() end");

    _event_in.clear();
    _event_out.clear();

    // godot::UtilityFunctions::print("AudioPluginHost::plugin_process() end");

    return 0;
}

int AudioPluginHost::process_note_on(int sample_offset, daw_event_t &event)
{
    spdlog::trace(__FUNCTION__);

    auto& daw_ev = event;

    godot::UtilityFunctions::print( std::format("AudioPluginHost::process_note_on() start: {} {} {} {}", sample_offset, event.channel, event.key, event.velocity).c_str() );
    spdlog::trace("{} {} {} {} {}", __FUNCTION__, sample_offset, event.channel, event.key, event.velocity);
    // checkForAudioThread();

/*
    公式ではどちらかというとCLAP_EVENT_NOTE_推奨らしい
    ノート イベントを送信する推奨方法は、CLAP_EVENT_NOTE_* を使用することです。
    The preferred way of sending a note event is to use CLAP_EVENT_NOTE_*.

    clap_event_midi midiev;
    // midi note on 144,64,90
    midiev.data[0] = 0x90 + 0;       //0x9xがノートオン
    midiev.data[1] = daw_ev.key;
    midiev.data[2] = daw_ev.velocity;
    midiev.port_index = 0;
    midiev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    midiev.header.type = CLAP_EVENT_MIDI;
    midiev.header.time = sample_offset;//daw_ev.event_time;
    midiev.header.flags = 0;
    midiev.header.size = sizeof(midiev);

    _event_in.push(&midiev.header);

    _event_in.push(&midiev.header);
*/


    clap_event_note ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_NOTE_ON;
    ev.header.time = sample_offset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.key = daw_ev.key;
    ev.channel = daw_ev.channel;
    ev.note_id = note_id;         // -1はダメらしい。0以上のインクリメントされたユニークを入れる
    ev.velocity = daw_ev.velocity / 127.0;

    _event_in.push(&ev.header);

    note_id++;

    spdlog::trace("{} end", __FUNCTION__);
    godot::UtilityFunctions::print("AudioPluginHost::process_note_on() end");

    return 0;
}

int AudioPluginHost::process_note_off(int sample_offset, daw_event_t &event)
{
    spdlog::trace(__FUNCTION__);

    godot::UtilityFunctions::print( std::format("AudioPluginHost::process_note_off() start: {} {} {} {}", sample_offset, event.channel, event.key, event.velocity).c_str() );
    spdlog::trace("{} {} {} {} {}", __FUNCTION__, sample_offset, event.channel, event.key, event.velocity);

    auto& daw_ev = event;
    //godot::UtilityFunctions::print( std::format("AudioPluginHost::process_note_off() start: {} {} {} {}", sample_offset, channel, key, velocity).c_str() );

    // checkForAudioThread();

/*
    公式ではどちらかというとCLAP_EVENT_NOTE_推奨らしい
    ノート イベントを送信する推奨方法は、CLAP_EVENT_NOTE_* を使用することです。
    The preferred way of sending a note event is to use CLAP_EVENT_NOTE_*.
    clap_event_midi midiev;
    
    midiev.data[0] = 0x80 + 0;       //0x8xがノートオフ
    midiev.data[1] = daw_ev.key;
    midiev.data[2] = daw_ev.velocity;
    midiev.port_index = 0;
    midiev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    midiev.header.type = CLAP_EVENT_MIDI;
    midiev.header.time = sample_offset;//daw_ev.event_time;
    midiev.header.flags = 0;
    midiev.header.size = sizeof(midiev);
    _event_in.push(&midiev.header);
    */

    clap_event_note ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_NOTE_OFF;
    ev.header.time = sample_offset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.key = daw_ev.key;
    ev.channel = daw_ev.channel;
    ev.note_id = daw_ev.note_id;
    ev.velocity = daw_ev.velocity / 127.0;

    _event_in.push(&ev.header);

    //note_id++;

    spdlog::trace("{} end", __FUNCTION__);
    godot::UtilityFunctions::print("AudioPluginHost::process_note_off() end");

    return 0;
}

int AudioPluginHost::process_param_change(int sample_offset, daw_event_t &event)
{
    spdlog::trace(__FUNCTION__);

    godot::UtilityFunctions::print( std::format("AudioPluginHost::process_param_change() start: {}", sample_offset).c_str() );
    spdlog::trace("{} {} {} {} {}", __FUNCTION__, sample_offset, event.channel, event.key, event.velocity);

    auto& daw_ev = event;
    //CLAP_EVENT_PARAM_VALUE:
    
    clap_event_param_value_t ev{};
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_PARAM_VALUE;
    ev.header.time = sample_offset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.value = daw_ev.param_value;
    ev.param_id = daw_ev.param_id;

    // 特定のキー、チャンネル、note_idを対象にする
    ev.value = daw_ev.param_value;
    ev.key = -1;//daw_ev.key;
    ev.channel = -1;//daw_ev.channel;
    ev.note_id = -1;//daw_ev.note_id;
    //ev.velocity = daw_ev.velocity / 127.0;

    _event_in.push(&ev.header);

    spdlog::trace("{} end", __FUNCTION__);
    godot::UtilityFunctions::print( std::format("AudioPluginHost::process_param_change() end").c_str() );
    return 0;
}

// ポート設定をAudioPluginHostのメンバ変数input_clap_audio_buffer等に設定
// 特にDawEngineから渡されるinputs、チャンネル数とかを設定している
void AudioPluginHost::set_ports(int num_inputs, float **inputs, int num_outputs, float **outputs)
{
    auto &in = input_clap_audio_buffer;
    auto &out = output_clap_audio_buffer;

    in.channel_count = num_inputs;
    in.data32 = inputs; // DawEngine::init()で設定された入力バッファ
    in.data64 = nullptr;
    in.constant_mask = 0;
    in.latency = 0;

    out.channel_count = num_outputs;
    out.data32 = outputs; // DawEngine::init()で設定された出力バッファ
    out.data64 = nullptr;
    out.constant_mask = 0;
    out.latency = 0;
}

int AudioPluginHost::get_clap_info(std::vector<std::filesystem::path> &sp)
{
    spdlog::trace(__FUNCTION__);
    godot::UtilityFunctions::print(std::format("get_clap_info() start").c_str());

    Json::Value res;
    for (const auto &q : sp)
    {
        Json::Value entryJson;

        auto h_module = LoadLibrary((LPCSTR)(q.generic_string().c_str()));
        if (!h_module)
            return -1;
        auto p_mod = GetProcAddress(h_module, "clap_entry");
        //    std::cout << "phan is " << phan << std::endl;
        auto entry = (clap_plugin_entry_t *)p_mod;

        if (entry)
        {
            entry->init(q.string().c_str());
            entryJson["path"] = q.string();
            entryJson["clap-version"] = std::to_string(entry->clap_version.major) + "." +
                                        std::to_string(entry->clap_version.minor) + "." +
                                        std::to_string(entry->clap_version.revision);
            entryJson["plugins"] = Json::Value();
            clap_scanner::foreachCLAPDescription(
                entry, [&entryJson](const clap_plugin_descriptor_t *desc)
                {
                    Json::Value thisPlugin;
                    thisPlugin["name"] = desc->name;
                    if (desc->version)
                        thisPlugin["version"] = desc->version;
                    thisPlugin["id"] = desc->id;
                    if (desc->vendor)
                        thisPlugin["vendor"] = desc->vendor;
                    if (desc->description)
                        thisPlugin["description"] = desc->description;

                    Json::Value features;

                    auto f = desc->features;
                    int idx = 0;
                    while (f[0])
                    {
                        bool nullWithinSize{false};
                        for (int i = 0; i < CLAP_NAME_SIZE; ++i)
                        {
                            if (f[0][i] == 0)
                            {
                                nullWithinSize = true;
                            }
                        }

                        if (!nullWithinSize)
                        {
                            std::cerr
                                << "Feature element at index " << idx
                                << " lacked null within CLAP_NAME_SIZE."
                                << "This means either a feature at this index overflowed or "
                                    "you didn't null terminate your "
                                << "features array" << std::endl;
                            break;
                        }
                        features.append(f[0]);
                        f++;
                        idx++;
                    }
                    thisPlugin["features"] = features;
                    entryJson["plugins"].append(thisPlugin); });
            res.append(entryJson);
            entry->deinit();

            // DLL を解放
            if (FreeLibrary(h_module))
            {
                std::cout << "DLL successfully unloaded" << std::endl;
            }
            else
            {
                std::cerr << "Failed to unload DLL" << std::endl;
            }

            godot::UtilityFunctions::print(std::format("clap info(): {}", entryJson.toStyledString()).c_str());
        }
    }

    godot::UtilityFunctions::print(std::format("get_clap_info() end").c_str());
    // doc.root["result"] = res;
    return 0;
}

// ホストコールバック関数
void AudioPluginHost::host_request_restart(const clap_host_t *) {}
void AudioPluginHost::host_request_process(const clap_host_t *) {}
void AudioPluginHost::host_request_callback(const clap_host_t *) {}
void AudioPluginHost::host_log(const clap_host_t *, clap_log_severity severity, const char *msg)
{
    std::cerr << "Plugin log: " << msg << std::endl;
}

/*
const void *AudioPluginHost::clapExtension(const clap_host *host, const char *extension) {
   //checkForMainThread();

   auto h = fromHost(host);

   if (!strcmp(extension, CLAP_EXT_GUI))
      return &h->_hostGui;
   if (!strcmp(extension, CLAP_EXT_LOG))
      return &h->_hostLog;
   if (!strcmp(extension, CLAP_EXT_THREAD_CHECK))
      return &h->_hostThreadCheck;
   if (!strcmp(extension, CLAP_EXT_THREAD_POOL))
      return &h->_hostThreadPool;
   if (!strcmp(extension, CLAP_EXT_TIMER_SUPPORT))
      return &h->_hostTimerSupport;
   if (!strcmp(extension, CLAP_EXT_POSIX_FD_SUPPORT))
      return &h->_hostPosixFdSupport;
   if (!strcmp(extension, CLAP_EXT_PARAMS))
      return &h->_hostParams;
   if (!strcmp(extension, CLAP_EXT_REMOTE_CONTROLS))
      return &h->_hostRemoteControls;
   if (!strcmp(extension, CLAP_EXT_STATE))
      return &h->_hostState;
   return nullptr;
}

AudioPluginHost *AudioPluginHost::fromHost(const clap_host *host) {
   if (!host)
      throw std::invalid_argument("Passed a null host pointer");

   auto h = static_cast<AudioPluginHost *>(host->host_data);
   if (!h)
      throw std::invalid_argument("Passed an invalid host pointer because the host_data is null");

   if (!h->clap_plugin)
      throw std::logic_error("The plugin can't query for extensions during the create method. Wait "
                             "for clap_plugin.init() call.");

   return h;
}
*/