#define NOMINMAX // clap-helperのmin/maxエラー対策

#include "common.hpp"
#include "gdtune.hpp"

#include <godot_cpp/classes/project_settings.hpp>

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
      spdlog::set_level(spdlog::level::debug);
      spdlog::info("spdlog/ GDTune() Construct start!");

      auto console = spdlog::stdout_color_mt("console");
      spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

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

      godot::String audio_plugin_dir = godot::ProjectSettings::get_singleton()->globalize_path("res://bin/audio_plugins/");
      std::string path_str = std::string(audio_plugin_dir.get_base_dir().utf8().get_data());
      godot::UtilityFunctions::print(std::format("audio_plugin_dir: {}", path_str).c_str());
      daw_engine.set_plugin_directory(path_str);

      // OSクラスを使用してパスを取得(godot::Engine::get_singleton()->is_editor_hint()
      // godot::String executable_path = godot::OS::get_singleton()->get_executable_path();
      // godot::String godot_project_path = godot::ProjectSettings::get_singleton()->globalize_path("res://");

      // DAWエンジンの初期化 ---------------------------------------------------
      // daw_engine.init();

      // オーディオデバイスの初期化 ---------------------------------------------------
   }

   // GDExtentionのデストラクタ
   GDTune::~GDTune()
   {
      LOG_FUNC_START();
      // エディタで実行されているかを確認し、エディタ内では開始処理をしない
      if (godot::Engine::get_singleton()->is_editor_hint())
      {
         return;
      }

      UtilityFunctions::print("~GDTune() start");

      // DAWエンジンの終了処理  ---------------------------------------------------
      // daw_engine.deinit();

      UtilityFunctions::print("~GDTune() end");
      LOG_FUNC_END();
   }

   void GDTune::init(godot::String plugin_dir, godot::String plugin_filename)
   {
      LOG_FUNC_START();
      std::string plugin_dir_str = std::string(plugin_dir.utf8().get_data());
      std::string plugin_filename_str = std::string(plugin_filename.utf8().get_data());

      daw_engine.init(plugin_dir_str, plugin_filename_str);
      LOG_FUNC_END();
   }

   void GDTune::deinit()
   {
      LOG_FUNC_START();
      daw_engine.deinit();
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
      // spdlog::trace(__FUNCTION__);
      LOG_FUNC_START();

      daw_engine.update(0.0);

      LOG_FUNC_END();
   }

   void GDTune::play_note(int key, double length, int velocity, int channel, double delay_time)
   {
      daw_engine.play_note(key, length, velocity, channel, delay_time);
   }

   void GDTune::add_note_on(int key, int velocity, int channel, double delay_time)
   {
      auto t = daw_engine.total_frames_processed + int64_t(delay_time * SAMPLE_RATE);
      daw_engine.add_note_on(t, key, velocity, channel);
   }

   void GDTune::add_note_off( int key, int velocity, int channel, double delay_time)
   {
      auto t = daw_engine.total_frames_processed + int64_t(delay_time * SAMPLE_RATE);
      daw_engine.add_note_off(t, key,  velocity, channel);
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
      LOG_FUNC_START();
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
      LOG_FUNC_START();
      auto json_str = daw_engine.get_clap_plugin_info();
      godot::String godot_json_str = godot::String(json_str.c_str());
      LOG_FUNC_END();

      return godot_json_str;
   }

   godot::String GDTune::get_loaded_plugin_params_json()
   {
      LOG_FUNC_START();

      //std::cout << daw_engine.loaded_plugin_params_json << std::endl;
      auto json_str = Json::FastWriter().write(daw_engine.loaded_plugin_params_json);
      auto s2 = json_str.c_str();
      //std::cout << "get_loaded_plugin_params_json: " << s2 << std::endl;
      godot::String godot_json_str = godot::String(s2);

      LOG_FUNC_END();

      return godot_json_str;
   }

   // GDExtensionノードとしての外部から利用できるAPIの登録
   void GDTune::_bind_methods()
   {
      LOG_FUNC_START();
      UtilityFunctions::print("GDTune::_bind_methods() start - Binding methods start.");

      ClassDB::bind_method(D_METHOD("init", "plugin_dir", "plugin_filename"), &GDTune::init);
      ClassDB::bind_method(D_METHOD("deinit"), &GDTune::deinit);
      ClassDB::bind_method(D_METHOD("update", "delta"), &GDTune::update);

      ClassDB::bind_method(D_METHOD("get_speed"), &GDTune::get_speed);
      ClassDB::bind_method(D_METHOD("set_speed", "speed"), &GDTune::set_speed);

      ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &GDTune::setFrequency);
      ClassDB::bind_method(D_METHOD("get_frequency"), &GDTune::getFrequency);
      ClassDB::bind_method(D_METHOD("get_instruments_info"), &GDTune::get_instruments_info);
      ClassDB::bind_method(D_METHOD("get_loaded_plugin_params_json"), &GDTune::get_loaded_plugin_params_json);

      ClassDB::bind_method(D_METHOD("play_note", "key", "length", "velocity", "channel", "delay_time"), &GDTune::play_note);
      ClassDB::bind_method(D_METHOD("add_note_on", "key", "velocity", "channel", "delay_time"), &GDTune::add_note_on);
      ClassDB::bind_method(D_METHOD("add_note_off", "key", "velocity", "channel", "delay_time"), &GDTune::add_note_off);
      ClassDB::bind_method(D_METHOD("param_change", "name", "value", "channel", "delay_time"), &GDTune::param_change);
      ClassDB::bind_method(D_METHOD("param_change_by_id", "param_id", "value", "channel", "delay_time"), &GDTune::param_change_by_id);

      ADD_GROUP("GDTune", "movement_");
      ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_speed"), "set_speed", "get_speed");

      UtilityFunctions::print("GDTune::_bind_methods() end Binding methods finished.");
   }

}
