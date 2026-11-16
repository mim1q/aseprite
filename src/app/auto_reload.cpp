// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/auto_reload.h"

#include "app/app.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/reload_file.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/debug.h"
#include "base/fs.h"
#include "doc/sprite.h"

#include <exception>
#include <string>
#include <vector>

namespace app {

AutoReload* AutoReload::m_instance = nullptr;

AutoReload::AutoReload(ui::Widget* owner) : m_timer(1000, owner)
{
  ASSERT(m_instance == nullptr);
  m_instance = this;

  if (UIContext* ctx = UIContext::instance()) {
    ctx->documents().add_observer(this);
    for (Doc* doc : ctx->documents())
      watchDoc(doc);
  }

  m_timer.Tick.connect([this] { onTick(); });
  m_timer.start();
}

AutoReload::~AutoReload()
{
  m_timer.stop();

  if (UIContext* ctx = UIContext::instance()) {
    for (const auto& item : m_stamps) {
      if (item.first)
        item.first->remove_observer(this);
    }
    ctx->documents().remove_observer(this);
  }

  m_stamps.clear();
  m_saving.clear();
  m_instance = nullptr;
}

void AutoReload::resetDoc(Doc* doc)
{
  if (doc)
    m_stamps[doc] = get_file_stamp(doc->filename());
}

void AutoReload::watchDoc(Doc* doc)
{
  if (!doc || m_stamps.find(doc) != m_stamps.end())
    return;

  doc->add_observer(this);
  m_stamps[doc] = get_file_stamp(doc->filename());
}

void AutoReload::unwatchDoc(Doc* doc)
{
  if (!doc)
    return;

  doc->remove_observer(this);
  m_stamps.erase(doc);
  m_saving.erase(doc);
}

void AutoReload::onAddDocument(Doc* doc)
{
  watchDoc(doc);
}

void AutoReload::onRemoveDocument(Doc* doc)
{
  unwatchDoc(doc);
}

void AutoReload::onBeforeSave(DocEvent& ev)
{
  if (ev.document())
    m_saving[ev.document()] = base::current_tick();
}

void AutoReload::onAfterSave(DocEvent& ev)
{
  Doc* doc = ev.document();
  if (!doc)
    return;

  m_saving.erase(doc);
  m_stamps[doc] = get_file_stamp(doc->filename());
}

void AutoReload::onFileNameChanged(Doc* doc)
{
  if (doc)
    m_stamps[doc] = get_file_stamp(doc->filename());
}

void AutoReload::refreshAllStamps()
{
  UIContext* ctx = UIContext::instance();
  if (!ctx)
    return;

  for (Doc* doc : ctx->documents()) {
    if (doc && doc->isAssociatedToFile() && !doc->filename().empty())
      m_stamps[doc] = get_file_stamp(doc->filename());
  }
}

void AutoReload::onTick()
{
  if (m_busy || !App::instance() || !App::instance()->isGui())
    return;

  UIContext* ctx = UIContext::instance();
  if (!ctx)
    return;

  if (!Preferences::instance().general.autoReloadFiles()) {
    refreshAllStamps();
    return;
  }

  std::vector<Doc*> docs;
  docs.reserve(ctx->documents().size());
  for (Doc* doc : ctx->documents())
    docs.push_back(doc);

  for (Doc* doc : docs) {
    if (!doc || doc->context() != ctx)
      continue;
    checkDoc(doc);
  }
}

void AutoReload::checkDoc(Doc* doc)
{
  auto savingIt = m_saving.find(doc);
  if (savingIt != m_saving.end()) {
    // Protect against a save operation that was cancelled and
    // therefore never notified us with onAfterSave().
    if (base::current_tick() - savingIt->second < 300000)
      return;

    m_saving.erase(savingIt);
    m_stamps[doc] = get_file_stamp(doc->filename());
    return;
  }

  if (doc->transaction())
    return;

  if (!doc->isAssociatedToFile() || doc->filename().empty())
    return;

  const FileStamp now = get_file_stamp(doc->filename());
  if (!now.exists)
    return;

  auto stampIt = m_stamps.find(doc);
  if (stampIt == m_stamps.end()) {
    m_stamps[doc] = now;
    return;
  }
  if (stampIt->second == now)
    return;

  if (!can_reload_document(doc)) {
    m_stamps[doc] = now;
    return;
  }

  // Do not interrupt a tool operation in progress.
  if (Editor* editor = UIContext::instance()->getEditorFor(doc)) {
    if (editor->getState() && editor->getState()->isTemporalState())
      return;
  }

  m_busy = true;
  std::string error;
  ReloadStatus status = ReloadStatus::Failed;
  try {
    status = reload_document(UIContext::instance(), doc, true, &error);
  }
  catch (const std::exception& e) {
    error = e.what();
  }
  m_busy = false;

  // Remember the stamp we observed before loading the file. If the
  // file changes again while it is being loaded, the next tick will
  // detect it and reload once more.
  m_stamps[doc] = now;

  if (status == ReloadStatus::Failed) {
    if (StatusBar* statusBar = StatusBar::instance()) {
      statusBar->setStatusText(
        3000,
        Strings::auto_reload_failed(base::get_file_name(doc->filename())));
    }
  }
}

} // namespace app
