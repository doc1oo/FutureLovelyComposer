#define NOMINMAX // clap-helperのmin/maxエラー対策

#include "gdtune.hpp"

#include <string>
#include <fstream>
#include <format>
#include <cassert>
#include <numbers>
#include <cmath>

namespace godot
{

   // GDExtentionのコンストラクタ
   GDTune::GDTune()
   {
      UtilityFunctions::print("-----------------------------------------------");
      UtilityFunctions::print("GDTune() Construct start!");
      // time_passed = 0;
      // sequencer.emitSignal = std::bind(&GDTune::emitSignal, this, std::placeholders::_1);

      // エディタで実行されているかを確認し、エディタ内では開始処理をしない
      if (godot::Engine::get_singleton()->is_editor_hint())
      {
         UtilityFunctions::print("is_editor_hint() == True and exit constructor!");
         return;
      }
      UtilityFunctions::print("is_editor_hint() == False.");

      // DAWエンジンの初期化 ---------------------------------------------------
      daw_engine.init();

      // オーディオデバイスの初期化 ---------------------------------------------------
   }

   // GDExtentionのデストラクタ
   GDTune::~GDTune()
   {
      // エディタで実行されているかを確認し、エディタ内では開始処理をしない
      if (godot::Engine::get_singleton()->is_editor_hint())
      {
         return;
      }

      UtilityFunctions::print("~GDTune() start");

      // DAWエンジンの終了処理  ---------------------------------------------------
      daw_engine.deinit();

      UtilityFunctions::print("~GDTune() end");
   }

   // GDTuneノードがGodotノードメインツリーに追加された場合、60FPSとかで自動的に呼ばれる処理
   void GDTune::_process(double_t delta)
   {
      Node::_process(delta);

      process_movement(delta);
   }

   // 60Hzを超えて可能な限り高速にくりかえし呼んで欲しい更新処理
   void GDTune::update(double_t delta)
   {

      daw_engine.update(0.0);
   }

   void GDTune::play_note(int key, double length, int velocity, int channel, double delay_time)
   {
      daw_engine.play_note(key, length, velocity, channel, delay_time);
   }

   void GDTune::param_change(godot::String name, double value, int channel, double delay_time)
   {
      daw_engine.add_param_change(name, value, channel, delay_time);
   }

   void GDTune::param_change_by_id(int param_id, double value, int channel, double delay_time)
   {
      daw_engine.add_param_change_by_id(param_id, value, channel, delay_time);
   }

   // デモ用の移動処理
   void GDTune::process_movement(double_t delta)
   {
      m_Velocity = Vector2(0.0f, 0.0f);
      Input &intutSingleton = *Input::get_singleton();

      if (intutSingleton.is_action_pressed("ui_right"))
      {
         m_Velocity.x += 1.0f;
      }

      if (intutSingleton.is_action_pressed("ui_left"))
      {
         m_Velocity.x -= 1.0f;
      }

      if (intutSingleton.is_action_pressed("ui_up"))
      {
         m_Velocity.y -= 1.0f;
      }

      if (intutSingleton.is_action_pressed("ui_down"))
      {
         m_Velocity.y += 1.0f;
      }

      set_position(get_position() + (m_Velocity * m_Speed * delta));
   }

   godot::String GDTune::get_instruments_info()
   {
      auto json_str = daw_engine.get_clap_plugin_info();
      godot::String godot_json_str = godot::String(json_str.c_str());
      return godot_json_str;
   }

   godot::String GDTune::get_loaded_plugin_params_json()
   {
      std::cout << "get_loaded_plugin_params_json() start " << std::endl;

      std::cout << daw_engine.loaded_plugin_params_json << std::endl;
      auto json_str =Json::FastWriter().write( daw_engine.loaded_plugin_params_json);
      auto s2 = json_str.c_str();
      std::cout << "get_loaded_plugin_params_json: " << s2 << std::endl;
      godot::String godot_json_str = godot::String( s2 );

      std::cout << "get_loaded_plugin_params_json() end " << std::endl;
      return godot_json_str;
   }

   // GDExtensionノードとしての外部から利用できるAPIの登録
   void GDTune::_bind_methods()
   {
      UtilityFunctions::print("Binding methods start.");

      ClassDB::bind_method(D_METHOD("get_speed"), &GDTune::get_speed);
      ClassDB::bind_method(D_METHOD("set_speed", "speed"), &GDTune::set_speed);

      ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &GDTune::setFrequency);
      ClassDB::bind_method(D_METHOD("get_frequency"), &GDTune::getFrequency);
      ClassDB::bind_method(D_METHOD("get_instruments_info"), &GDTune::get_instruments_info);
      ClassDB::bind_method(D_METHOD("get_loaded_plugin_params_json"), &GDTune::get_loaded_plugin_params_json);

      ClassDB::bind_method(D_METHOD("update", "delta"), &GDTune::update);
      ClassDB::bind_method(D_METHOD("play_note", "key", "length", "velocity", "channel", "delay_time"), &GDTune::play_note);
      ClassDB::bind_method(D_METHOD("param_change", "name", "value", "channel", "delay_time"), &GDTune::param_change);
      ClassDB::bind_method(D_METHOD("param_change_by_id", "param_id", "value", "channel", "delay_time"), &GDTune::param_change_by_id);

      ADD_GROUP("GDTune", "movement_");
      ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_speed"), "set_speed", "get_speed");

      UtilityFunctions::print("Binding methods finished.");
   }

}

/*
void PluginHost::process() {
   checkForAudioThread();

   if (!_plugin)
      return;

   // Can't process a plugin that is not active
   if (!isPluginActive())
      return;

   // Do we want to deactivate the plugin?
   if (_scheduleDeactivate) {
      _scheduleDeactivate = false;
      if (_state == ActiveAndProcessing)
         _plugin->stop_processing(_plugin);
      setPluginState(ActiveAndReadyToDeactivate);
      return;
   }

   // We can't process a plugin which failed to start processing
   if (_state == ActiveWithError)
      return;

   _process.transport = nullptr;

   _process.in_events = _evIn.clapInputEvents();
   _process.out_events = _evOut.clapOutputEvents();

   _process.audio_inputs = &_audioIn;
   _process.audio_inputs_count = 1;
   _process.audio_outputs = &_audioOut;
   _process.audio_outputs_count = 1;

   _evOut.clear();
   generatePluginInputEvents();

   if (isPluginSleeping()) {
      if (!_scheduleProcess && _evIn.empty())
         // The plugin is sleeping, there is no request to wake it up and there are no events to
         // process
         return;

      _scheduleProcess = false;
      if (!_plugin->start_processing(_plugin)) {
         // the plugin failed to start processing
         setPluginState(ActiveWithError);
         return;
      }

      setPluginState(ActiveAndProcessing);
   }

   int32_t status = CLAP_PROCESS_SLEEP;
   if (isPluginProcessing())
      status = _plugin->process(_plugin, &_process);

   handlePluginOutputEvents();

   _evOut.clear();
   _evIn.clear();

   _engineToAppValueQueue.producerDone();

   // TODO: send plugin to sleep if possible

   g_thread_type = ThreadType::Unknown;
}
*/

/*



int main() {
    MinimalClapHost host;

    // オーディオデバイスの初期化
    if (!host.initAudio()) {
        std::cerr << "Failed to initialize audio" << std::endl;
        return 1;
    }

    // プラグインのロード
    if (!host.loadPlugin("path/to/your/plugin.clap")) {
        std::cerr << "Failed to load plugin" << std::endl;
        return 1;
    }

    // オーディオ処理の開始
    if (!host.start()) {
        std::cerr << "Failed to start audio processing" << std::endl;
        return 1;
    }

    std::cout << "Playing audio... Press Enter to stop." << std::endl;
    std::cin.get();

    // 終了処理
    host.stop();

    return 0;
}


*/