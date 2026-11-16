// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_FILE_STAMP_H_INCLUDED
#define APP_UTIL_FILE_STAMP_H_INCLUDED
#pragma once

#include <cstdint>
#include <string>

namespace app {

// A cheap snapshot of a file used to detect external modifications.
struct FileStamp {
  bool exists = false;
  uint64_t mtime = 0;
  uint64_t size = 0;

  bool operator==(const FileStamp& other) const
  {
    return (exists == other.exists && mtime == other.mtime && size == other.size);
  }

  bool operator!=(const FileStamp& other) const { return !(*this == other); }
};

FileStamp get_file_stamp(const std::string& filename);

} // namespace app

#endif
