/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup asset_system
 */

#pragma once

#include <optional>

#include "AS_asset_library.hh"

#include "BLI_function_ref.hh"
#include "BLI_map.hh"

#include <memory>

struct AssetLibraryReference;
struct bUserAssetLibrary;

namespace blender::asset_system {

/**
 * Global singleton-ish that provides access to individual #AssetLibrary instances.
 *
 * Whenever a blend file is loaded, the existing instance of AssetLibraryService is destructed, and
 * a new one is created -- hence the "singleton-ish". This ensures only information about relevant
 * asset libraries is loaded.
 *
 * \note How Asset libraries are identified may change in the future.
 *  For now they are assumed to be:
 * - on disk (identified by the absolute directory), or
 * - the "current file" library (which is in memory but could have catalogs
 *   loaded from a file on disk).
 */
class AssetLibraryService {
  static std::unique_ptr<AssetLibraryService> instance_;

  /* Mapping absolute path of the library's root path (normalize with #normalize_directory_path()!)
   * the AssetLibrary instance. */
  Map<std::string, std::unique_ptr<AssetLibrary>> on_disk_libraries_;
  /** Library without a known path, i.e. the "Current File" library if the file isn't saved yet. If
   * the file was saved, a valid path for the library can be determined and #on_disk_libraries_
   * above should be used. */
  std::unique_ptr<AssetLibrary> current_file_library_;
  /** The "all" asset library, merging all other libraries into one. */
  std::unique_ptr<AssetLibrary> all_library_;

  /* Handlers for managing the life cycle of the AssetLibraryService instance. */
  bCallbackFuncStore on_load_callback_store_;
  static bool atexit_handler_registered_;

 public:
  AssetLibraryService() = default;
  ~AssetLibraryService() = default;

  /** Return the AssetLibraryService singleton, allocating it if necessary. */
  static AssetLibraryService *get();

  /** Destroy the AssetLibraryService singleton. It will be reallocated by #get() if necessary. */
  static void destroy();

  static std::string root_path_from_library_ref(const AssetLibraryReference &library_reference);
  static bUserAssetLibrary *find_custom_asset_library_from_library_ref(
      const AssetLibraryReference &library_reference);
  static bUserAssetLibrary *find_custom_preferences_asset_library_from_asset_weak_ref(
      const AssetWeakReference &asset_reference);

  AssetLibrary *get_asset_library(const Main *bmain,
                                  const AssetLibraryReference &library_reference);

  /** Get an asset library of type #ASSET_LIBRARY_CUSTOM. */
  AssetLibrary *get_asset_library_on_disk_custom(StringRef name, StringRefNull root_path);
  /** Get a builtin (not user defined) asset library. I.e. a library that is **not** of type
   * #ASSET_LIBRARY_CUSTOM. */
  AssetLibrary *get_asset_library_on_disk_builtin(eAssetLibraryType type, StringRefNull root_path);
  /** Get the "Current File" asset library. */
  AssetLibrary *get_asset_library_current_file();
  /** Get the "All" asset library, which loads all others and merges them into one. */
  AssetLibrary *get_asset_library_all(const Main *bmain);

  /** Get a valid library path from the weak reference. Empty if e.g. the reference is to a local
   * asset. */
  std::string resolve_asset_weak_reference_to_library_path(
      const AssetWeakReference &asset_reference);
  /* See #AS_asset_full_path_resolve_from_weak_ref(). */
  std::string resolve_asset_weak_reference_to_full_path(const AssetWeakReference &asset_reference);
  /** Struct to hold results from path explosion functions
   * (#resolve_asset_weak_reference_to_exploded_path()). */
  struct ExplodedPath {
    /* The string buffer containing the fully resolved path, if resolving was successful. */
    std::string full_path = "";
    /* Reference into the part of #full_path that is the directory path. */
    StringRef dir_component = "";
    /* Reference into the part of #full_path that is the ID group name ("Object", "Brush", ...). */
    StringRef group_component = "";
    /* Reference into the part of #full_path that is the ID name. */
    StringRef name_component = "";
  };
  /** Similar to #BKE_blendfile_library_path_explode, returns the full path as
   * #resolve_asset_weak_reference_to_library_path, with StringRefs to the `dir` (i.e. blendfile
   * path), `group` (i.e. ID type) and `name` (i.e. ID name) parts. */
  std::optional<ExplodedPath> resolve_asset_weak_reference_to_exploded_path(
      const AssetWeakReference &asset_reference);

  /** Returns whether there are any known asset libraries with unsaved catalog edits. */
  bool has_any_unsaved_catalogs() const;

  /** See AssetLibrary::foreach_loaded(). */
  void foreach_loaded_asset_library(FunctionRef<void(AssetLibrary &)> fn,
                                    bool include_all_library) const;

 protected:
  /** Allocate a new instance of the service and assign it to `instance_`. */
  static void allocate_service_instance();

  AssetLibrary *find_loaded_on_disk_asset_library_from_name(StringRef name) const;

  /**
   * Get the given asset library. Opens it (i.e. creates a new AssetLibrary instance) if necessary.
   */
  AssetLibrary *get_asset_library_on_disk(eAssetLibraryType library_type,
                                          StringRef name,
                                          StringRefNull top_level_directory);
  /**
   * Ensure the AssetLibraryService instance is destroyed before a new blend file is loaded.
   * This makes memory management simple, and ensures a fresh start for every blend file. */
  void app_handler_register();
  void app_handler_unregister();
};

}  // namespace blender::asset_system
