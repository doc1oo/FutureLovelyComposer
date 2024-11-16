#pragma once

#include "common.hpp"

// #define MINIAUDIO_IMPLEMENTATION
/*
#include <clap/clap.h>
#include "clap/entry.h"
#include "clap/host.h"
#include "clap/events.h"

#include <clap/ext/latency.h>
#include <clap/ext/gui.h>
#include <clap/ext/state.h>

#include "clap/ext/params.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/note-ports.h"
#include "clap/factory/plugin-factory.h"
*/

// #include "CLI11/CLI11.hpp"

#include <cstdint>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>

#include "daw_engine.hpp"

/*
namespace ProjectInfo
{
    const char* const  projectName = "GDTune";
    const char* const  companyName = "Individual";
    const char* const  versionString = "0.0.1";
    const int          versionNumber = 0x10000;
}
#endif
*/

namespace godot
{

  class GDTune : public Node2D
  {
    GDCLASS(GDTune, Node2D)

  public:
    GDTune();
    ~GDTune();

    // Will be called by Godot when the class is registered
    // Use this to add properties to your class
    static void _bind_methods();

    void _process(double_t delta) override;

    // property setter
    void set_speed(float_t speed)
    {
      m_Speed = speed;
    }

    // property getter
    [[nodiscard]] float_t get_speed() const
    {
      return m_Speed;
    }

    [[nodiscard]] godot::String get_instruments_info();
    godot::String get_loaded_plugin_params_json();

    // property setter
    void setFrequency(float_t speed)
    {
      frequency = speed;
      daw_engine.wave_form.advance = frequency / 10000.0;
      daw_engine.wave_form.config.amplitude = 0.2 + frequency / 1000.0;
    }

    // property getter
    [[nodiscard]] float_t getFrequency() const
    {
      return frequency;
    }

    void update(double_t delta);
    void play_note(int key, double length, int velocity, int channel, double delay_time);
    void param_change(godot::String name, double value, int channel, double delay_time);
    void param_change_by_id(int param_id, double value, int channel, double delay_time);
    void init(godot::String plugin_dir, godot::String plugin_filename);
    void deinit();

    // バインディング用のメソッド群を自動生成するマクロを定義
    /*
    #define FORWARD_METHOD(return_type, method_name, ...) \
        return_type method_name(__VA_ARGS__) { \
            return daw_engine.method_name(__VA_ARGS__); \
    }


    FORWARD_METHOD(void, add_param_change, godot::String name, double value, int channel, double delay_time );
    */

  private:
    DawEngine daw_engine;
    Vector2 m_Velocity;

    
    // This will be a property later (look into _bind_methods)
    float_t m_Speed = 500.0f;
    double frequency = 440.0;

    void process_movement(double_t delta);

  };

}