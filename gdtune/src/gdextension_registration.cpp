#define NOMINMAX // clap-helperのmin/maxエラー対策

#include "gdtune.hpp"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

void register_gdtune_types(godot::ModuleInitializationLevel p_level)
{

  if (p_level != godot::ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SCENE)
  {
    return;
  }

  godot::ClassDB::register_class<godot::GDTune>();
}

void unregister_gdtune_types(godot::ModuleInitializationLevel p_level)
{

  // DO NOTHING
  if (p_level != godot::ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SCENE)
  {
    return;
  }
}

extern "C"
{

  GDExtensionBool GDE_EXPORT gdtune_library_init(
      GDExtensionInterfaceGetProcAddress p_get_proc_address,
      GDExtensionClassLibraryPtr p_library,
      GDExtensionInitialization *r_initialization)
  {

    godot::GDExtensionBinding::InitObject init_object(p_get_proc_address, p_library, r_initialization);

    init_object.register_initializer(register_gdtune_types);
    init_object.register_terminator(unregister_gdtune_types);
    init_object.set_minimum_library_initialization_level(godot::ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_object.init();
  }
}