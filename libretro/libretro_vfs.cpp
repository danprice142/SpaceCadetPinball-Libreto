#include "libretro_vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#define stat _stat
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#else
#include <unistd.h>
#include <dirent.h>
#endif

// VFS constants - define locally if not in libretro.h
#ifndef RETRO_VFS_FILE_ACCESS_READ
#define RETRO_VFS_FILE_ACCESS_READ            (1 << 0)
#define RETRO_VFS_FILE_ACCESS_WRITE           (1 << 1)
#define RETRO_VFS_FILE_ACCESS_READ_WRITE      (RETRO_VFS_FILE_ACCESS_READ | RETRO_VFS_FILE_ACCESS_WRITE)
#define RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING (1 << 2)
#define RETRO_VFS_FILE_ACCESS_HINT_NONE       (0)
#define RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS (1 << 0)
#define RETRO_VFS_SEEK_POSITION_START    0
#define RETRO_VFS_SEEK_POSITION_CURRENT  1
#define RETRO_VFS_SEEK_POSITION_END      2
#define RETRO_VFS_STAT_IS_VALID          (1 << 0)
#define RETRO_VFS_STAT_IS_DIRECTORY      (1 << 1)
#endif

// VFS file handle structure
struct retro_vfs_file_handle
{
	FILE* fp;
	char* path;
	unsigned mode;
	unsigned hints;
};

// VFS directory handle structure  
struct retro_vfs_dir_handle
{
#ifdef _WIN32
	WIN32_FIND_DATAA find_data;
	HANDLE find_handle;
	bool first_entry;
#else
	DIR* dir;
	struct dirent* entry;
#endif
	char* path;
};

// Forward declarations of VFS functions
static const char* retro_vfs_file_get_path_impl(struct retro_vfs_file_handle* stream);
static struct retro_vfs_file_handle* retro_vfs_file_open_impl(const char* path, unsigned mode, unsigned hints);
static int retro_vfs_file_close_impl(struct retro_vfs_file_handle* stream);
static int64_t retro_vfs_file_size_impl(struct retro_vfs_file_handle* stream);
static int64_t retro_vfs_file_truncate_impl(struct retro_vfs_file_handle* stream, int64_t length);
static int64_t retro_vfs_file_tell_impl(struct retro_vfs_file_handle* stream);
static int64_t retro_vfs_file_seek_impl(struct retro_vfs_file_handle* stream, int64_t offset, int seek_position);
static int64_t retro_vfs_file_read_impl(struct retro_vfs_file_handle* stream, void* s, uint64_t len);
static int64_t retro_vfs_file_write_impl(struct retro_vfs_file_handle* stream, const void* s, uint64_t len);
static int retro_vfs_file_flush_impl(struct retro_vfs_file_handle* stream);
static int retro_vfs_file_remove_impl(const char* path);
static int retro_vfs_file_rename_impl(const char* old_path, const char* new_path);
static int retro_vfs_stat_impl(const char* path, int32_t* size);
static int retro_vfs_mkdir_impl(const char* dir);
static struct retro_vfs_dir_handle* retro_vfs_opendir_impl(const char* dir, bool include_hidden);
static bool retro_vfs_readdir_impl(struct retro_vfs_dir_handle* dirstream);
static const char* retro_vfs_dirent_get_name_impl(struct retro_vfs_dir_handle* dirstream);
static bool retro_vfs_dirent_is_dir_impl(struct retro_vfs_dir_handle* dirstream);
static int retro_vfs_closedir_impl(struct retro_vfs_dir_handle* dirstream);

// VFS interface structure
static struct retro_vfs_interface vfs_interface = {
	retro_vfs_file_get_path_impl,
	retro_vfs_file_open_impl,
	retro_vfs_file_close_impl,
	retro_vfs_file_size_impl,
	retro_vfs_file_tell_impl,
	retro_vfs_file_seek_impl,
	retro_vfs_file_read_impl,
	retro_vfs_file_write_impl,
	retro_vfs_file_flush_impl,
	retro_vfs_file_remove_impl,
	retro_vfs_file_rename_impl,
	retro_vfs_file_truncate_impl,
	retro_vfs_stat_impl,
	retro_vfs_mkdir_impl,
	retro_vfs_opendir_impl,
	retro_vfs_readdir_impl,
	retro_vfs_dirent_get_name_impl,
	retro_vfs_dirent_is_dir_impl,
	retro_vfs_closedir_impl
};

static struct retro_vfs_interface_info vfs_interface_info = {
	3,
	&vfs_interface
};

// Implementation of VFS functions

static const char* retro_vfs_file_get_path_impl(struct retro_vfs_file_handle* stream)
{
	if (!stream)
		return nullptr;
	return stream->path;
}

static struct retro_vfs_file_handle* retro_vfs_file_open_impl(const char* path, unsigned mode, unsigned hints)
{
	if (!path)
		return nullptr;

	struct retro_vfs_file_handle* handle = (struct retro_vfs_file_handle*)calloc(1, sizeof(struct retro_vfs_file_handle));
	if (!handle)
		return nullptr;

	const char* mode_str = nullptr;
	if (mode & RETRO_VFS_FILE_ACCESS_READ_WRITE)
	{
		if (mode & RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING)
			mode_str = "r+b";
		else
			mode_str = "w+b";
	}
	else if (mode & RETRO_VFS_FILE_ACCESS_READ)
		mode_str = "rb";
	else if (mode & RETRO_VFS_FILE_ACCESS_WRITE)
	{
		if (mode & RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING)
			mode_str = "r+b";
		else
			mode_str = "wb";
	}

	if (!mode_str)
	{
		free(handle);
		return nullptr;
	}

	handle->fp = fopen(path, mode_str);
	if (!handle->fp)
	{
		free(handle);
		return nullptr;
	}

	handle->path = strdup(path);
	handle->mode = mode;
	handle->hints = hints;

	return handle;
}

static int retro_vfs_file_close_impl(struct retro_vfs_file_handle* stream)
{
	if (!stream)
		return -1;

	if (stream->fp)
		fclose(stream->fp);

	if (stream->path)
		free(stream->path);

	free(stream);
	return 0;
}

static int64_t retro_vfs_file_size_impl(struct retro_vfs_file_handle* stream)
{
	if (!stream || !stream->fp)
		return -1;

	int64_t current_pos = ftell(stream->fp);
	fseek(stream->fp, 0, SEEK_END);
	int64_t size = ftell(stream->fp);
	fseek(stream->fp, current_pos, SEEK_SET);

	return size;
}

static int64_t retro_vfs_file_truncate_impl(struct retro_vfs_file_handle* stream, int64_t length)
{
	if (!stream || !stream->fp)
		return -1;

#ifdef _WIN32
	int fd = _fileno(stream->fp);
	if (_chsize_s(fd, length) != 0)
		return -1;
#else
	int fd = fileno(stream->fp);
	if (ftruncate(fd, length) != 0)
		return -1;
#endif

	return 0;
}

static int64_t retro_vfs_file_tell_impl(struct retro_vfs_file_handle* stream)
{
	if (!stream || !stream->fp)
		return -1;

	return ftell(stream->fp);
}

static int64_t retro_vfs_file_seek_impl(struct retro_vfs_file_handle* stream, int64_t offset, int seek_position)
{
	if (!stream || !stream->fp)
		return -1;

	int whence = SEEK_SET;
	switch (seek_position)
	{
	case RETRO_VFS_SEEK_POSITION_START:
		whence = SEEK_SET;
		break;
	case RETRO_VFS_SEEK_POSITION_CURRENT:
		whence = SEEK_CUR;
		break;
	case RETRO_VFS_SEEK_POSITION_END:
		whence = SEEK_END;
		break;
	default:
		return -1;
	}

	if (fseek(stream->fp, offset, whence) != 0)
		return -1;

	return ftell(stream->fp);
}

static int64_t retro_vfs_file_read_impl(struct retro_vfs_file_handle* stream, void* s, uint64_t len)
{
	if (!stream || !stream->fp || !s)
		return -1;

	return fread(s, 1, len, stream->fp);
}

static int64_t retro_vfs_file_write_impl(struct retro_vfs_file_handle* stream, const void* s, uint64_t len)
{
	if (!stream || !stream->fp || !s)
		return -1;

	return fwrite(s, 1, len, stream->fp);
}

static int retro_vfs_file_flush_impl(struct retro_vfs_file_handle* stream)
{
	if (!stream || !stream->fp)
		return -1;

	return fflush(stream->fp);
}

static int retro_vfs_file_remove_impl(const char* path)
{
	if (!path)
		return -1;

	return remove(path);
}

static int retro_vfs_file_rename_impl(const char* old_path, const char* new_path)
{
	if (!old_path || !new_path)
		return -1;

	return rename(old_path, new_path);
}

static int retro_vfs_stat_impl(const char* path, int32_t* size)
{
	if (!path)
		return 0;

	struct stat buf;
	if (stat(path, &buf) != 0)
		return 0;

	int flags = RETRO_VFS_STAT_IS_VALID;

	if (S_ISDIR(buf.st_mode))
		flags |= RETRO_VFS_STAT_IS_DIRECTORY;

	if (size)
		*size = (int32_t)buf.st_size;

	return flags;
}

static int retro_vfs_mkdir_impl(const char* dir)
{
	if (!dir)
		return -1;

#ifdef _WIN32
	return _mkdir(dir);
#else
	return mkdir(dir, 0755);
#endif
}

static struct retro_vfs_dir_handle* retro_vfs_opendir_impl(const char* dir, bool include_hidden)
{
	if (!dir)
		return nullptr;

	struct retro_vfs_dir_handle* handle = (struct retro_vfs_dir_handle*)calloc(1, sizeof(struct retro_vfs_dir_handle));
	if (!handle)
		return nullptr;

	handle->path = strdup(dir);

#ifdef _WIN32
	char search_path[MAX_PATH];
	snprintf(search_path, sizeof(search_path), "%s\\*", dir);
	
	handle->find_handle = FindFirstFileA(search_path, &handle->find_data);
	if (handle->find_handle == INVALID_HANDLE_VALUE)
	{
		free(handle->path);
		free(handle);
		return nullptr;
	}
	handle->first_entry = true;
#else
	handle->dir = opendir(dir);
	if (!handle->dir)
	{
		free(handle->path);
		free(handle);
		return nullptr;
	}
#endif

	return handle;
}

static bool retro_vfs_readdir_impl(struct retro_vfs_dir_handle* dirstream)
{
	if (!dirstream)
		return false;

#ifdef _WIN32
	if (dirstream->first_entry)
	{
		dirstream->first_entry = false;
		return true;
	}
	
	return FindNextFileA(dirstream->find_handle, &dirstream->find_data) != 0;
#else
	dirstream->entry = readdir(dirstream->dir);
	return dirstream->entry != nullptr;
#endif
}

static const char* retro_vfs_dirent_get_name_impl(struct retro_vfs_dir_handle* dirstream)
{
	if (!dirstream)
		return nullptr;

#ifdef _WIN32
	return dirstream->find_data.cFileName;
#else
	if (!dirstream->entry)
		return nullptr;
	return dirstream->entry->d_name;
#endif
}

static bool retro_vfs_dirent_is_dir_impl(struct retro_vfs_dir_handle* dirstream)
{
	if (!dirstream)
		return false;

#ifdef _WIN32
	return (dirstream->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	if (!dirstream->entry)
		return false;
	return dirstream->entry->d_type == DT_DIR;
#endif
}

static int retro_vfs_closedir_impl(struct retro_vfs_dir_handle* dirstream)
{
	if (!dirstream)
		return -1;

#ifdef _WIN32
	if (dirstream->find_handle != INVALID_HANDLE_VALUE)
		FindClose(dirstream->find_handle);
#else
	if (dirstream->dir)
		closedir(dirstream->dir);
#endif

	if (dirstream->path)
		free(dirstream->path);

	free(dirstream);
	return 0;
}

// Public API implementation

bool libretro_vfs_init(void)
{
	return true;
}

struct retro_vfs_interface_info* libretro_vfs_get_interface(void)
{
	return &vfs_interface_info;
}
