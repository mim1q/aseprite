// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_AUTO_RELOAD_H_INCLUDED
#define APP_AUTO_RELOAD_H_INCLUDED
#pragma once

#include "app/doc_observer.h"
#include "app/docs_observer.h"
#include "app/util/file_stamp.h"
#include "base/time.h"
#include "ui/timer.h"

#include <map>

namespace ui {
class Widget;
}

namespace app {

class Doc;

// Periodically checks the files associated to all open documents and
// reloads them when they are modified by another program. Documents
// with unsaved changes are never reloaded without asking.
class AutoReload : public DocsObserver, public DocObserver {
public:
  explicit AutoReload(ui::Widget* owner);
  ~AutoReload();

  static AutoReload* instance() { return m_instance; }

  // Updates the last known file stamp of the document (e.g. after a
  // manual reload).
  void resetDoc(Doc* doc);

private:
  void onTick();
  void watchDoc(Doc* doc);
  void unwatchDoc(Doc* doc);
  void checkDoc(Doc* doc);
  void refreshAllStamps();

  void onAddDocument(Doc* doc) override;
  void onRemoveDocument(Doc* doc) override;
  void onBeforeSave(DocEvent& ev) override;
  void onAfterSave(DocEvent& ev) override;
  void onFileNameChanged(Doc* doc) override;

  ui::Timer m_timer;
  std::map<Doc*, FileStamp> m_stamps;
  std::map<Doc*, base::tick_t> m_saving;
  bool m_busy = false;

  static AutoReload* m_instance;
};

} // namespace app

#endif
