#define MINIAUDIO_IMPLEMENTATION
#define NOMINMAX // clap-helperのmin/maxエラー対策

#include <iostream>
#include <thread>
#include <chrono>
#include <numbers>
#include <format>
#include <vector>
#include <ranges>
#include <algorithm>
#include <string>

#include "daw_engine.hpp"

double g_phase = 0;

// pInputに音が入ってくるし、pOutputに音を出力する
// プラグインの音を取ってきたかったらdaw_engineかaudio_plugin_hostのオーディオバッファを書き換えて、それをコピーするなどする？
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{ // 再生モードでは、データを pOutput にコピーします。キャプチャ モードでは、データを pInput から読み取ります。全二重モードでは、pOutput と pInput の両方が有効になり、データを pInput から pOutput に移動できます。frameCount を超えるフレームは処理しないでください。

    auto daw_engine = static_cast<DawEngine *>(pDevice->pUserData);

    if (daw_engine->is_processing)
    {
        // フレーム数から時間を計算
        //double streamTime = pDevice->playback.proc.currentFrame / (double)pDevice->sampleRate;

        //double streamTime = timeInFrames / (double)pDevice->sampleRate;
 
        double streamTime = daw_engine->total_frames_processed / (double)pDevice->sampleRate;

        daw_engine->extract_upcoming_events();

        daw_engine->process_audio(static_cast<const float *>(pInput), static_cast<float *>(pOutput), frameCount, streamTime);
    }

    // std::cout << frameCount << std::endl; 1024とか

    for (int i = 0; i < frameCount; i++)
    {
        // ma_int16* output = (ma_int16*)pOutput;
        ma_float *output = (ma_float *)pOutput;

        output[i * 2 + 0] = daw_engine->daw_audio_output_buffer[i];               // 左チャンネル
        output[i * 2 + 1] = daw_engine->daw_audio_output_buffer[i + BUFFER_SIZE]; // 右チャンネル
    }

    daw_engine->total_frames_processed += frameCount;
}

/*
// Miniaudioの波形ジェネレータを使ったコールバック関数
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    ma_waveform *pWaveForm;
    ma_uint64 framesRead;

    // godot::godot::UtilityFunctions::print(std::format("frameCount: {}", frameCount).c_str() );
    // std::cout << std::format("frameCount: {}", frameCount).c_str() << std::endl;

    MA_ASSERT(pDevice->playback.channels == DEVICE_CHANNELS);
    if (pDevice->playback.channels != DEVICE_CHANNELS)
    {
        godot::godot::UtilityFunctions::print(std::format("pDevice->playback.channels == DEVICE_CHANNELS").c_str());
    }

    pWaveForm = (ma_waveform *)pDevice->pUserData;
    MA_ASSERT(pWaveForm != NULL);
    if (pWaveForm == NULL)
    {
        godot::godot::UtilityFunctions::print(std::format("pWaveForm != NULL").c_str());
    }

    // auto result = ma_waveform_read_pcm_frames(pWaveForm, pOutput, frameCount, NULL);
    auto result = ma_waveform_read_pcm_frames(pWaveForm, pOutput, frameCount, &framesRead);
    if (result != MA_SUCCESS)
    {
        godot::godot::UtilityFunctions::print(std::format("Failed to read PCM frames from waveform. {}", framesRead).c_str());
    }
}
*/

DawEngine::DawEngine() : is_processing(false)
{
    // DAWオーディオバッファの初期化
    daw_audio_input_buffer.resize(BUFFER_SIZE * 2); // ステレオ
    daw_audio_output_buffer.resize(BUFFER_SIZE * 2);

    /*
    const float* inputs[2] = {&daw_audio_input_buffer[0], &daw_audio_input_buffer[BUFFER_SIZE]};
    float* outputs[2] = {&daw_audio_output_buffer[0], &daw_audio_output_buffer[BUFFER_SIZE]};

    audio_input = &daw_audio_input_buffer[0];   // LR
    audio_output = &daw_audio_output_buffer[0]; // LR*/
    /*
    inputs[1] = &inputBuffer[BUFFER_SIZE];      // R
    outputs[0] = &outputBuffer[0];
    outputs[1] = &outputBuffer[BUFFER_SIZE];
    */
}

DawEngine::~DawEngine()
{
    /*
    stop();

    if (plugin) {
        plugin->destroy(plugin);
    }
    if (library) {
        dlclose(library);
    }

    ma_device_uninit(&device);
    */
}

int DawEngine::init(std::string plugin_dir, std::string plugin_filename)
{
    godot::UtilityFunctions::print(std::format("DawEngine::init() start").c_str());

    if (plugin_dir != "") {
        plugin_dir_path = plugin_dir;
    }
    if (plugin_filename != "") {
        clap_file_name = plugin_filename;
    }

    // CLAPプラグインのファイルパスを作成
    std::vector<std::filesystem::path> clap_file_pathes;
    std::string path_str = plugin_dir_path;
    if (!path_str.ends_with('/')) {
        path_str += "/";
    }
    path_str += clap_file_name;
    clap_file_pathes.push_back(std::filesystem::path(path_str));

    device_open_result = init_audio();
    init_midi();

    audio_plugin_host.init();

    audio_plugin_host.init_clap_plugin(clap_file_pathes);
    
    loaded_plugin_params_json = audio_plugin_host.get_loaded_plugin_params_json();

    auto num = loaded_plugin_params_json["param-count"].asUInt();
    std::cout << loaded_plugin_params_json["param-info"].toStyledString() << std::endl;

    godot::UtilityFunctions::print(std::format("Plugin parameter num: {}", num).c_str());
    godot::UtilityFunctions::print(std::format("Plugin parameters ----------").c_str());
    for(int i=0; i<num; i++) {
        auto& prm = loaded_plugin_params_json["param-info"][i];
        //auto& root = json["root"];
        //json["param-info"][i]["name"];
        
        godot::UtilityFunctions::print(std::format("[{}] ... current={} default={} min={} max={}", prm["name"].asString(), prm["values"]["current"].asString(),prm["values"]["default"].asString(), prm["values"]["min"].asString(), prm["values"]["max"].asString()).c_str());
        
    }

    _inputs[0] = &daw_audio_input_buffer[0];
    _inputs[1] = &daw_audio_input_buffer[BUFFER_SIZE];
    _outputs[0] = &daw_audio_output_buffer[0];
    _outputs[1] = &daw_audio_output_buffer[BUFFER_SIZE];

    audio_plugin_host.set_ports(2, _inputs, 2, _outputs);
    audio_plugin_host.plugin_activate(device_sample_rate, BUFFER_SIZE);
    // audio_plugin_host.get_clap_info(clap_file_pathes);

    is_processing = true;

    godot::UtilityFunctions::print(std::format("DawEngine::init() end").c_str());

    return 0;
}

int DawEngine::init_audio()
{
    godot::UtilityFunctions::print(std::format("init_audio_device() start").c_str());

    const bool ENABLE_WASAPI_LOW_LATENCY_MODE = false;

    // コンテキスト設定の初期化 ---------------------------------------------------
    context_config = ma_context_config_init();
    context_config.threadPriority = ma_thread_priority_highest;
    // context_config.alsa.useVerboseDeviceEnumeration = MA_TRUE;		// ALSAの場合のみ

    // WASAPIを使用するための設定 ---------------------------------------------------
    // ma_backend backends[] = { ma_backend_wasapi, ma_backend_dsound, ma_backend_winmm };
    ma_backend backends[] = {ma_backend_wasapi};

    if (ma_context_init(backends, 3, &context_config, &context) != MA_SUCCESS)
    {
        godot::UtilityFunctions::print(std::format("ma_context_init failed.").c_str());
    }

    ma_device_info *pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info *pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
    {
        godot::UtilityFunctions::print(std::format("ma_context_get_devices failed.").c_str());
    }

    // Loop over each device info and do something with it. Here we just print the name with their index. You may want
    // to give the user the opportunity to choose which device they'd prefer.
    for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1)
    {
        printf("%d - %s\n", iDevice, pPlaybackInfos[iDevice].name);
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback); // 再生モードで初期化

    // config.playback.pDeviceID を数値指定することで以下のようなデバイスを選択できる
    // 0 - OUT (UA-3FX)
    // 1 - DELL S2722DC (NVIDIA High Definition Audio)
    config.playback.pDeviceID = &pPlaybackInfos[selected_audio_playback_device_index].id; // オーディオデバイス選択

    // config.playback.pDeviceID = NULL;  // NULLでデフォルトデバイスを使用
    // config.playback.pDeviceID = &context.pDeviceInfos->id;  // NULLでデフォルトデバイスを使用
    // config.playback.format = DEVICE_FORMAT;   // Set to ma_format_unknown to use the device's native format.
    // config.playback.channels = DEVICE_CHANNELS;               // Set to 0 to use the device's native channel count.
    // config.sampleRate = DEVICE_SAMPLE_RATE;           // Set to 0 to use the device's native sample rate.
    config.dataCallback = data_callback; // This function will be called when miniaudio needs more data.
    // config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).
    // config.pUserData = &wave_form;
    config.pUserData = this;
    config.periodSizeInFrames = 1024;

    // WASAPIを使用するように指定
    if (ENABLE_WASAPI_LOW_LATENCY_MODE)
    {
        config.wasapi.noAutoConvertSRC = MA_TRUE;     // オプション: サンプルレート変換を無効化
        config.wasapi.noDefaultQualitySRC = MA_TRUE;  // オプション: デフォルトの品質設定を使用しない
        config.wasapi.noAutoStreamRouting = MA_TRUE;  // オプション: 自動ストリームルーティングを無効化
        config.wasapi.noHardwareOffloading = MA_TRUE; // オプション: ハードウェアオフロードを無効化
    }

    // デバイスの初期化 ---------------------------------------------------
    godot::UtilityFunctions::print(std::format("ma_device_init() start").c_str());
    if (ma_device_init(&context, &config, &device) != MA_SUCCESS)
    {
        godot::UtilityFunctions::print(std::format("ma_device_init failed.").c_str());
        return -1; // Failed to initialize the device.
    }

    godot::UtilityFunctions::print(std::format("Device Name: {}", device.playback.name).c_str());

    // 波形ジェネレータの初期化 ---------------------------------------------------
    wave_form_config = ma_waveform_config_init(
        device.playback.format,
        device.playback.channels,
        device.sampleRate,
        ma_waveform_type_square,
        0.2,
        440);
    godot::UtilityFunctions::print(std::format("ma_waveform_config_init done.").c_str());

    ma_result result = ma_waveform_init(&wave_form_config, &wave_form);
    if (result != MA_SUCCESS)
    {
        godot::UtilityFunctions::print(std::format("ma_waveform_init: failed").c_str());
    }

    godot::UtilityFunctions::print(std::format("wave_form.config.frequency: {}", wave_form.config.frequency).c_str());
    godot::UtilityFunctions::print(std::format("ma_waveform_init done.").c_str());

    // デバイスの開始 ---------------------------------------------------
    godot::UtilityFunctions::print(std::format("ma_device_start() start").c_str());
    godot::UtilityFunctions::print(std::format("Device Name: {}", device.playback.name).c_str());
    // ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        godot::UtilityFunctions::print("Failed to start playback device.");
        ma_device_uninit(&device);
        return -5;
    }

    godot::UtilityFunctions::print(std::format("miniaudio Initialising all successed.").c_str());

    return 0;
}

int DawEngine::init_midi()
{
    godot::UtilityFunctions::print(std::format("init_midi_device() start").c_str());

    godot::UtilityFunctions::print(std::format("RtMidiIn construct").c_str());
    try
    {
        midiin = new RtMidiIn();
        godot::UtilityFunctions::print(std::format("RtMidiIn construct success.").c_str());
    }
    catch (RtMidiError &error)
    {
        // Handle the exception here
        error.printMessage();
        is_input_init_success = false;
    }

    godot::UtilityFunctions::print(std::format("is_input_init_success: {}", is_input_init_success).c_str());
    // Check inputs.
    if (is_input_init_success == true)
    {
        std::cout << "Check inputs start." << " \n";
        unsigned int nPorts = 0;
        try
        {
            nPorts = midiin->getPortCount();
            std::cout << "midiin->getPortCount(): " << nPorts << " \n";
        }
        catch (RtMidiError &error)
        {
            error.printMessage();
        }
        std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
        std::string portName;
        for (unsigned int i = 0; i < nPorts; i++)
        {
            try
            {
                portName = midiin->getPortName(i);
                std::cout << "  Input Port #" << i + 1 << ": " << portName << '\n';
            }
            catch (RtMidiError &error)
            {
                error.printMessage();
            }
        }
        midiin->openPort(selected_input_midi_port);

        // Don't ignore sysex, timing, or active sensing messages.
        // midiin->ignoreTypes( false, false, false );
        midiin->ignoreTypes(false, true, true);
        // midiin->setBufferSize(1024, 4);
    }

    godot::UtilityFunctions::print(std::format("RtMidiOut construct").c_str());
    // RtMidiOut constructor
    try
    {
        midiout = new RtMidiOut(RtMidi::WINDOWS_MM);
        godot::UtilityFunctions::print(std::format("RtMidiOut construct success.").c_str());
    }
    catch (RtMidiError &error)
    {
        error.printMessage();
        is_output_init_success = false;
    }

    // Check outputs.
    if (is_output_init_success)
    {
        std::cout << "Check outputs start." << " \n";
        unsigned int nPorts = 0;
        nPorts = midiout->getPortCount();
        std::cout << "\nThere are " << nPorts << " MIDI output ports available.\n";
        std::string portName;
        for (unsigned int i = 0; i < nPorts; i++)
        {
            try
            {
                portName = midiout->getPortName(i);
                std::cout << "  Output Port #" << i + 1 << ": " << portName << '\n';
            }
            catch (RtMidiError &error)
            {
                error.printMessage();
            }
        }
        std::cout << '\n';

        // ----------------------------------------------------------------------
        std::vector<unsigned char> message;

        // 利用可能な最初のポートを開きます。
        midiout->openPort(selected_output_midi_port);

        // Send out a series of MIDI messages.

        /*
        GM1システム・オン：F0,7E,7F,09,01,F7
        XG System on F0,43,10,4C,00,00,7E,00,F7
        */
        message.clear();
        message.push_back(0xF0);
        message.push_back(0x7E);
        message.push_back(0x7F);
        message.push_back(0x09);
        message.push_back(0x01);
        message.push_back(0xF7);
        midiout->sendMessage(&message);

        message.clear();
        // Program change: 192, 5
        message.push_back(192);
        message.push_back(5);
        midiout->sendMessage(&message);

        // Control Change: 176, 7, 100 (volume)
        message.clear();
        message.push_back(176);
        message.push_back(7);
        message.push_back(100);
        midiout->sendMessage(&message);

        /*
        // Note On: 144, 64, 90
        message[0] = 144;
        message[1] = 64;
        message[2] = 90;
        midiout->sendMessage( &message );
        */

        /*
        SLEEP( 500 ); // Platform-dependent ... see example in tests directory.

        // Note Off: 128, 64, 40
        message[0] = 128;
        message[1] = 64;
        message[2] = 40;
        midiout->sendMessage( &message );
        */
    }

    godot::UtilityFunctions::print(std::format("init_midi_device() end").c_str());

    return 0;
}

int DawEngine::deinit()
{
    is_processing = false;

    godot::UtilityFunctions::print(std::format("DawEngine::deinit() start").c_str());
    audio_plugin_host.deinit();

    deinit_midi();
    deinit_audio();

    godot::UtilityFunctions::print(std::format("DawEngine::deinit() end").c_str());
    return 0;
}

int DawEngine::deinit_audio()
{

    // --------------------------------------------------------------------
    godot::UtilityFunctions::print("Closing audio device start.");

    // delete [] pcmBuf;
    // pcmBuf = nullptr;
    // adm.closeAudioDevice();

    if (device_open_result >= 0)
    {
        auto result = ma_device_stop(&device);
        if (result != MA_SUCCESS)
        {
            godot::UtilityFunctions::print("ma_device_stop() failed.");
        }
        ma_device_uninit(&device);
    }

    godot::UtilityFunctions::print("Closing audio device finished.");

    return 0;
}

int DawEngine::deinit_midi()
{

    // --------------------------------------------------------------------
    godot::UtilityFunctions::print("Closing midi device start.");

    // Clean up
    if (is_input_init_success)
    {
        godot::UtilityFunctions::print(std::format("delete midiin start").c_str());
        delete midiin;
    }
    if (is_output_init_success)
    {
        godot::UtilityFunctions::print(std::format("delete midiout start").c_str());
        delete midiout;
    }

    godot::UtilityFunctions::print("Closing midi device finished.");
    return 0;
}

bool DawEngine::load_plugin(const char *path)
{
    /*
    library = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    if (!library) {
        std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
        return false;
    }

    auto entry = (clap_plugin_entry_t*)dlsym(library, "clap_entry");
    if (!entry) {
        std::cerr << "Failed to find clap_entry symbol" << std::endl;
        return false;
    }

    if (!entry->init(path)) {
        std::cerr << "Failed to initialize plugin" << std::endl;
        return false;
    }

    const clap_plugin_factory_t* factory =
        static_cast<const clap_plugin_factory_t*>(
            entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (!factory) {
        std::cerr << "Failed to get plugin factory" << std::endl;
        return false;
    }

    uint32_t count = factory->get_plugin_count(factory);
    if (count == 0) {
        std::cerr << "No plugins found" << std::endl;
        return false;
    }

    plugin = factory->create_plugin(factory, &host, factory->get_plugin_id(factory, 0));
    if (!plugin) {
        std::cerr << "Failed to create plugin instance" << std::endl;
        return false;
    }

    if (!plugin->init(plugin)) {
        std::cerr << "Failed to initialize plugin instance" << std::endl;
        return false;
    }
    */

    return true;
}

bool DawEngine::start()
{
    /*
    if (!plugin) {
        std::cerr << "No plugin loaded" << std::endl;
        return false;
    }

    // プラグインのアクティベート
    plugin->activate(plugin, SAMPLE_RATE, BUFFER_SIZE, BUFFER_SIZE);

    // オーディオデバイスの開始
    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cerr << "Failed to start audio device" << std::endl;
        return false;
    }

    isProcessing = true;
    */
    return true;
}

void DawEngine::stop()
{
    /*
    isProcessing = false;
    if (plugin) {
        plugin->deactivate(plugin);
    }

    ma_device_stop(&device);
    */
}

int DawEngine::update(double delta)
{

    // エディタで実行されているかを確認し、エディタ内では開始処理をしない
    // if (godot::Engine::get_singleton()->is_editor_hint()) {
    //	return;
    //}

    // tone_generator.setFrequency(frequency);

    int nBytes, i;
    double time_stamp;

    // Periodically check input queue.
    std::vector<unsigned char> receive_mes;
    std::vector<unsigned char> send_mes;
    // std::cout << "Reading MIDI from port ... quit with Ctrl-C.\n";

    while (time_stamp = midiin->getMessage(&receive_mes))
    {

        if (receive_mes.empty())
            break;

        nBytes = receive_mes.size();

        // To Midi-Out ----------------------------------------------
        send_mes.clear();
        for (i = 0; i < nBytes; i++)
        {
            // std::cout << "Byte" << i << "=" << (int)message[i] << ", ";
            send_mes.push_back(receive_mes[i]);
        }
        midiout->sendMessage(&send_mes);

        if (nBytes > 0)
        {
            // std::cout << "time_stamp=" << time_stamp << std::endl;
        }

        // To CLAP Plugin ----------------------------------------------
        // auto msgTime = thiz->_midiIn->getMessage(&midiBuf);

        uint8_t eventType = receive_mes[0] >> 4;
        uint8_t channel = receive_mes[0] & 0xf;
        uint8_t data1 = receive_mes[1];
        uint8_t data2 = receive_mes[2];

        // double deltaMs = currentTime - time_stamp;
        double deltaSample = (delta * device_sample_rate) / 1000;

        // if (deltaSample >= frame_count)
        //	deltaSample = frame_count - 1;

        int frame_count = 100; // 一時的にエラーを回避するための仮の値

        int32_t sampleOffset = 0; // frame_count - deltaSample;

        godot::UtilityFunctions::print(std::format("DawEngine::update() {},{},{},{}", sampleOffset, channel, data1, data2).c_str());


        daw_event_t dawev;
        dawev.event_time = sampleOffset;
        dawev.channel = channel;
        dawev.key = data1;
        dawev.velocity = data2;

        switch (eventType)
        {

        case MIDI_STATUS_NOTE_ON:
            audio_plugin_host.process_note_on(sampleOffset, dawev);
            godot::UtilityFunctions::print("MIDI_STATUS_NOTE_ON.");
            break;

        case MIDI_STATUS_NOTE_OFF:
            audio_plugin_host.process_note_off(sampleOffset, dawev);
            godot::UtilityFunctions::print("MIDI_STATUS_NOTE_OFF.");
            break;

        case MIDI_STATUS_CC:
            // thiz->_pluginHost->processCC(sampleOffset, channel, data1, data2);
            break;

        default:
            std::cerr << "unknown event type: " << (int)eventType << std::endl;
            break;
        }
    }

    // printf("test %f\n", delta);
    // printf("frequency %f\n", frequency);
    // wave_form.config.frequency = frequency;
    // wave_form.advance

    /*

    thiz->_pluginHost->process();

    // copy output
    for (int i = 0; i < frameCount; ++i) {
        out[2 * i] = thiz->_outputs[0][i];
        out[2 * i + 1] = thiz->_outputs[1][i];
    }

    thiz->_steadyTime += frameCount;

    switch (thiz->_state) {
    case DawEngine::kStateRunning:
        return 0;
    case DawEngine::kStateStopping:
        thiz->_state = DawEngine::kStateStopped;
        return 1;
    default:
        assert(false && "unreachable");
        return 2;
    }
    */

    return 0;
}

void DawEngine::process_audio(const float *input, float *output, uint32_t frame_count, double stream_time)
{

    // godot::UtilityFunctions::print(std::format("DawEngine::process_audio() start").c_str() );
    /*
     if (!audio_plugin_host || !isProcessing)
     {
         // プラグインが無効な場合は無音を出力
         std::fill(output, output + frameCount * 2, 0.0f);
         return;
     }*/

    // 入力バッファをクリア（無音入力）
    std::fill(daw_audio_input_buffer.begin(), daw_audio_input_buffer.end(), 0.0f);

    /*
    clap_process_t process{};
    clap_audio_buffer_t in_buffer{};
    clap_audio_buffer_t out_buffer{};
    */

    if (audio_plugin_host.clap_plugin)
    {
        audio_plugin_host.plugin_process(input, output, frame_count, stream_time);
    }
    else
    {
        godot::UtilityFunctions::print(std::format("DawEngine:: audio_plugin_host.clap_plugin in null.").c_str());
    }

    // in_buffer.data32 = const_cast<float **>(audio_input);
    // in_buffer.channel_count = 2;
    // out_buffer.data32 = audio_output;
    // out_buffer.channel_count = 2;
    /*
        process.audio_inputs = &in_buffer;
        process.audio_inputs_count = 1;
        process.audio_outputs = &out_buffer;
        process.audio_outputs_count = 1;
        process.frames_count = frameCount;

        // プラグインで処理
        plugin->process(plugin, &process);

        // インターリーブされた出力形式に変換
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            output[i * 2] = outputs[0][i];     // Left channel
            output[i * 2 + 1] = outputs[1][i]; // Right channel
        }
        */
    // godot::UtilityFunctions::print(std::format("DawEngine::process_audio() end").c_str() );
}

void DawEngine::set_plugin_directory(std::string plugin_dir)
{
    plugin_dir_path = plugin_dir;
}

void DawEngine::play_note(int key, double length, int velocity, int channel, double delay_time)
{
    if (!is_processing)
    {
        return;
    }

    uint64_t start_frame =  total_frames_processed + int64_t(delay_time * SAMPLE_RATE);
    uint64_t end_frame =  start_frame + int64_t(length * SAMPLE_RATE);
    std::cout << "start_frame: " << start_frame << std::endl;
    std::cout << "end_frame: " << end_frame << std::endl;

    add_note_on(start_frame, key, velocity, channel);
    add_note_off(end_frame, key, velocity, channel);
    note_id++;

}


int DawEngine::add_note_on(uint64_t event_time, int key, int velocity, int channel)
{
    godot::UtilityFunctions::print("DawEngine::add_note_on() start");
    // checkForAudioThread();

    daw_event_t ev;
    ev.type = DAW_EVENT_NOTE_ON;
    ev.event_time = event_time;
    ev.key = key;
    ev.velocity = velocity;
    ev.note_id = note_id;

    _daw_events.push_back(ev);

    godot::UtilityFunctions::print("DawEngine::add_note_on() end");
    return 0;
}

int DawEngine::add_note_off(uint64_t event_time, int key, int velocity, int channel)
{
    godot::UtilityFunctions::print("DawEngine::add_note_off() start");
    // checkForAudioThread();

    daw_event_t ev;
    ev.type = DAW_EVENT_NOTE_OFF;
    ev.event_time = event_time;
    ev.key = key;
    ev.velocity = velocity;
    ev.note_id = note_id;

    _daw_events.push_back(ev);

    godot::UtilityFunctions::print("DawEngine::add_note_off end");
    return 0;
}


int DawEngine::add_param_change_by_id(int param_id, double value, int channel, double delay_time) {
    godot::UtilityFunctions::print("DawEngine::add_param_change_by_id() start");

    uint64_t start_frame =  total_frames_processed + int64_t(delay_time * SAMPLE_RATE);

    godot::UtilityFunctions::print("DawEngine::add_note_off() start");
    // checkForAudioThread();

    daw_event_t ev;
    ev.type = DAW_EVENT_PARAM_CHANGE;
    ev.event_time = start_frame;
    ev.channel = channel;
    ev.param_value = value;
    ev.param_id = param_id;
    //ev.key = key;
    //ev.note_id = note_id;

    _daw_events.push_back(ev);

    godot::UtilityFunctions::print("DawEngine::add_param_change_by_id() end");
    return 0;
}

int DawEngine::add_param_change(godot::String name, double value, int channel, double delay_time)
{
    godot::UtilityFunctions::print("DawEngine::add_param_change() start");
    std::string cstring_name = name.utf8().get_data();

    int param_id = -1;
    auto num = loaded_plugin_params_json["param-count"].asUInt();
    for(int i=0; i<num; i++) {
        auto& prm = loaded_plugin_params_json["param-info"][i];
        //std::cout << "cstr: " << cstring_name << " prmname:" << prm["name"].asString() << std::endl;
        if (strcmp(cstring_name.c_str(), prm["name"].asString().c_str()) == 0) {     // 0: equal
            //auto& root = json["root"];
            try {
                std::cout << prm["id"] << std::endl;
                param_id = prm["id"].asInt();
            } catch (std::exception& e) {
                std::cout << "Exception: " << e.what() << std::endl;
                godot::UtilityFunctions::print("Exception.");
                return -1;
            }
            
            //godot::UtilityFunctions::print(std::format("[{}] ... current={} default={} min={} max={}", prm["name"].asString(), prm["values"]["current"].asString(),prm["values"]["default"].asString(), prm["values"]["min"].asString(), prm["values"]["max"].asString()).c_str());
            break;
        }
    }

    std::cout << "param_id: " << param_id << std::endl;

    if (param_id == -1) {
        godot::UtilityFunctions::print( std::format("param_id not found.: {}", param_id).c_str() );
        return -1;
    }
    godot::UtilityFunctions::print("param_id found. {}");

    add_param_change_by_id(param_id, value, channel, delay_time);

    godot::UtilityFunctions::print("DawEngine::add_param_change end");
    return 0;
}

std::string DawEngine::get_clap_plugin_info()
{
    auto& info = audio_plugin_host.get_clap_plugin_info();

    std::string json_str = Json::FastWriter().write(info.root);

    // std::stringをGodot::Stringに変換して渡す
    godot::String godot_json_str = godot::String(json_str.c_str());

    // Godot側で JSON.parse() を使用して変換
    // GDScriptなど
    //var data = JSON.parse(godot_json_str);
    
    godot::UtilityFunctions::print("get_clap_plugin_info() end");

    return json_str;
}

void DawEngine::extract_upcoming_events() {
    //godot::UtilityFunctions::print("extract_upcoming_events() start");

    uint64_t start_frame = total_frames_processed;
    uint64_t end_frame = start_frame + BUFFER_SIZE - 1;

    for(int i=0; i < _daw_events.size(); i++) {

        auto& ev = _daw_events[i];

        if (ev.event_time >= start_frame && ev.event_time < end_frame) {
            uint64_t sample_offset = ev.event_time - start_frame;

            if(ev.type == DAW_EVENT_NOTE_ON) {
                audio_plugin_host.process_note_on(sample_offset, ev);
            } else if(ev.type == DAW_EVENT_NOTE_OFF) {
                audio_plugin_host.process_note_off(sample_offset, ev);
            } else if(ev.type == DAW_EVENT_PARAM_CHANGE) {
                audio_plugin_host.process_param_change(sample_offset, ev);
            }
           // _daw_events.erase(_daw_events.begin() + i);
           ev.erace_flag = true;
        }

    }

    std::erase_if(_daw_events, [](daw_event_t ev) { return ev.erace_flag; });

    //godot::UtilityFunctions::print("extract_upcoming_events() end");
}

/*

miniaucioの波形ジェネレータを使う版
int GDTune::init_audio_device()
{
    ma_device_config deviceConfig;
    ma_device device;

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate        = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback      = data_callback;
    //deviceConfig.pUserData         = &sineWave;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        return -1;
    }

    // printf("Device Name: %s\n", device.playback.name);
    // godot::UtilityFunctions::print("Failed to open playback device.");
    godot::UtilityFunctions::print(std::format("Device Name: {}", device.playback.name).c_str() );

    sineWaveConfig = ma_waveform_config_init(device.playback.format, device.playback.channels, device.sampleRate, ma_waveform_type_sine, 0.2, 220);

    godot::UtilityFunctions::print( std::format("ma_waveform_config_init done.").c_str() );

    ma_waveform_init(&sineWaveConfig, &sineWave);


    //godot::UtilityFunctions::print( std::format("ma_waveform_init done.").c_str() );

    if (ma_device_start(&device) != MA_SUCCESS) {
        godot::UtilityFunctions::print("Failed to start playback device.");
        ma_device_uninit(&device);
        return -5;
    }

    ma_device_start(&device);
    //godot::UtilityFunctions::print( std::format("Initialising all successed.").c_str() );

    return 0;
}
*/
