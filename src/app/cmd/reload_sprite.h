// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_RELOAD_SPRITE_H_INCLUDED
#define APP_CMD_RELOAD_SPRITE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/file/format_options.h"
#include "doc/frame.h"

#include <cstddef>
#include <string>

namespace doc {
class Sprite;
} // namespace doc

namespace app {
class Doc;

namespace cmd {

// Replaces the whole sprite of a document by a new one loaded from
// the disk, keeping the old sprite alive so the operation can be
// undone.
class ReloadSprite : public Cmd {
public:
  ReloadSprite(Doc* doc,
               doc::Sprite* newSprite,
               const FormatOptionsPtr& newFormatOptions,
               const bool newReadOnly);
  ~ReloadSprite() override;

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  void onFireNotifications() override;
  std::string onLabel() const override;
  size_t onMemSize() const override;

private:
  void replaceSprite(doc::Sprite* sprite,
                     const FormatOptionsPtr& formatOptions,
                     const bool readOnly);

  Doc* m_doc;
  doc::Sprite* m_oldSprite = nullptr;
  doc::Sprite* m_newSprite = nullptr;
  doc::Sprite* m_ownedSprite = nullptr;
  FormatOptionsPtr m_oldFormatOptions;
  FormatOptionsPtr m_newFormatOptions;
  bool m_oldReadOnly = false;
  bool m_newReadOnly = false;
};

} // namespace cmd
} // namespace app

#endif
