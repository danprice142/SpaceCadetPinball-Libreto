#pragma once

#include "libretro.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for VFS structures
struct retro_vfs_file_handle;
struct retro_vfs_dir_handle;

// VFS interface function pointers
typedef const char* (RETRO_CALLCONV *retro_vfs_get_path_t)(struct retro_vfs_file_handle* stream);
typedef struct retro_vfs_file_handle* (RETRO_CALLCONV *retro_vfs_open_t)(const char* path, unsigned mode, unsigned hints);
typedef int (RETRO_CALLCONV *retro_vfs_close_t)(struct retro_vfs_file_handle* stream);
typedef int64_t (RETRO_CALLCONV *retro_vfs_size_t)(struct retro_vfs_file_handle* stream);
typedef int64_t (RETRO_CALLCONV *retro_vfs_tell_t)(struct retro_vfs_file_handle* stream);
typedef int64_t (RETRO_CALLCONV *retro_vfs_seek_t)(struct retro_vfs_file_handle* stream, int64_t offset, int seek_position);
typedef int64_t (RETRO_CALLCONV *retro_vfs_read_t)(struct retro_vfs_file_handle* stream, void* s, uint64_t len);
typedef int64_t (RETRO_CALLCONV *retro_vfs_write_t)(struct retro_vfs_file_handle* stream, const void* s, uint64_t len);
typedef int (RETRO_CALLCONV *retro_vfs_flush_t)(struct retro_vfs_file_handle* stream);
typedef int (RETRO_CALLCONV *retro_vfs_remove_t)(const char* path);
typedef int (RETRO_CALLCONV *retro_vfs_rename_t)(const char* old_path, const char* new_path);
typedef int64_t (RETRO_CALLCONV *retro_vfs_truncate_t)(struct retro_vfs_file_handle* stream, int64_t length);
typedef int (RETRO_CALLCONV *retro_vfs_stat_t)(const char* path, int32_t* size);
typedef int (RETRO_CALLCONV *retro_vfs_mkdir_t)(const char* dir);
typedef struct retro_vfs_dir_handle* (RETRO_CALLCONV *retro_vfs_opendir_t)(const char* dir, bool include_hidden);
typedef bool (RETRO_CALLCONV *retro_vfs_readdir_t)(struct retro_vfs_dir_handle* dirstream);
typedef const char* (RETRO_CALLCONV *retro_vfs_dirent_get_name_t)(struct retro_vfs_dir_handle* dirstream);
typedef bool (RETRO_CALLCONV *retro_vfs_dirent_is_dir_t)(struct retro_vfs_dir_handle* dirstream);
typedef int (RETRO_CALLCONV *retro_vfs_closedir_t)(struct retro_vfs_dir_handle* dirstream);

// VFS interface structure (API version 3)
struct retro_vfs_interface
{
    retro_vfs_get_path_t get_path;
    retro_vfs_open_t open;
    retro_vfs_close_t close;
    retro_vfs_size_t size;
    retro_vfs_tell_t tell;
    retro_vfs_seek_t seek;
    retro_vfs_read_t read;
    retro_vfs_write_t write;
    retro_vfs_flush_t flush;
    retro_vfs_remove_t remove;
    retro_vfs_rename_t rename;
    retro_vfs_truncate_t truncate;
    retro_vfs_stat_t stat;
    retro_vfs_mkdir_t mkdir;
    retro_vfs_opendir_t opendir;
    retro_vfs_readdir_t readdir;
    retro_vfs_dirent_get_name_t dirent_get_name;
    retro_vfs_dirent_is_dir_t dirent_is_dir;
    retro_vfs_closedir_t closedir;
};

// VFS interface info structure
struct retro_vfs_interface_info
{
    uint32_t required_interface_version;
    struct retro_vfs_interface* iface;
};

// Initialize VFS interface - call in retro_init()
bool libretro_vfs_init(void);

// Get VFS interface - called by frontend via RETRO_ENVIRONMENT_GET_VFS_INTERFACE
struct retro_vfs_interface_info* libretro_vfs_get_interface(void);

#ifdef __cplusplus
}
#endif
