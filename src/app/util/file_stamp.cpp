// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/file_stamp.h"

#if LAF_WINDOWS
  #include <windows.h>
#else
  #include <sys/stat.h>
#endif

namespace app {

FileStamp get_file_stamp(const std::string& filename)
{
  FileStamp stamp;

#if LAF_WINDOWS
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (!GetFileAttributesExA(filename.c_str(), GetFileExInfoStandard, &data))
    return stamp;

  ULARGE_INTEGER writeTime;
  writeTime.LowPart = data.ftLastWriteTime.dwLowDateTime;
  writeTime.HighPart = data.ftLastWriteTime.dwHighDateTime;

  stamp.exists = true;
  stamp.mtime = writeTime.QuadPart * 100; // 100ns units -> nanoseconds
  stamp.size = (uint64_t(data.nFileSizeHigh) << 32) | uint64_t(data.nFileSizeLow);
#else
  struct stat st;
  if (stat(filename.c_str(), &st) != 0)
    return stamp;

  #if __APPLE__
  stamp.mtime =
    uint64_t(st.st_mtimespec.tv_sec) * 1000000000ull + uint64_t(st.st_mtimespec.tv_nsec);
  #else
  stamp.mtime = uint64_t(st.st_mtim.tv_sec) * 1000000000ull + uint64_t(st.st_mtim.tv_nsec);
  #endif
  stamp.exists = true;
  stamp.size = uint64_t(st.st_size);
#endif

  return stamp;
}

} // namespace app
