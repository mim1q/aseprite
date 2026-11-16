// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RELOAD_FILE_H_INCLUDED
#define APP_RELOAD_FILE_H_INCLUDED
#pragma once

#include <string>

namespace app {

class Context;
class Doc;

enum class ReloadStatus {
  Reloaded,
  Canceled,
  Failed,
};

// Returns true if the document can be reloaded from its original
// file. Multi-file sequences and untitled documents cannot.
bool can_reload_document(const Doc* doc);

// Reloads the given document from its file. The operation is recorded
// in the document undo history (the previous sprite can be restored
// with Undo). If askIfModified is true and the document has unsaved
// changes, a confirmation dialog is shown (only when the UI is
// available).
ReloadStatus reload_document(Context* ctx, Doc* doc, bool askIfModified, std::string* error = nullptr);

} // namespace app

#endif
