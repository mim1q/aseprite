// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/reload_file.h"

#include "app/cmd/reload_sprite.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/i18n/strings.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/sprite.h"
#include "ui/alert.h"

#include <exception>
#include <memory>

namespace app {

bool can_reload_document(const Doc* doc)
{
  if (!doc || !doc->isAssociatedToFile() || doc->filename().empty())
    return false;

  if (!base::is_file(doc->filename()))
    return false;

  if (doc->sprites().size() != 1)
    return false;

  const std::string ext = base::string_to_lower(base::get_file_extension(doc->filename()));
  if (ext == "aseprite" || ext == "ase")
    return true;

  // Reloading only the first file of a multi-file animation sequence
  // would destroy the rest of the frames.
  return (doc->sprite() && doc->sprite()->totalFrames() == 1);
}

static bool ask_to_reload(Doc* doc)
{
  ui::Alert alert;
  alert.setTitle(Strings::auto_reload_title());
  alert.addLabel(Strings::auto_reload_message(doc->filename()), ui::LEFT);
  alert.addLabel(Strings::auto_reload_question(), ui::LEFT);
  alert.addButton(Strings::auto_reload_reload());
  alert.addButton(Strings::auto_reload_keep());
  return alert.show() == 1;
}

ReloadStatus reload_document(Context* ctx, Doc* doc, bool askIfModified, std::string* error)
{
  if (!doc || !can_reload_document(doc)) {
    if (error)
      *error = Strings::auto_reload_unsupported();
    return ReloadStatus::Failed;
  }

  if (askIfModified && doc->isModified() && ctx && ctx->isUIAvailable()) {
    if (!ask_to_reload(doc))
      return ReloadStatus::Canceled;
  }

  const std::string filename = doc->filename();

  try {
    std::unique_ptr<FileOp> fop(FileOp::createLoadDocumentOperation(
      nullptr,
      filename,
      FILE_LOAD_CREATE_PALETTE | FILE_LOAD_SEQUENCE_NONE));
    if (!fop) {
      if (error)
        *error = Strings::auto_reload_unsupported();
      return ReloadStatus::Failed;
    }

    fop->operate();
    fop->done();
    fop->postLoad();

    if (fop->hasError()) {
      if (error)
        *error = fop->error();
      return ReloadStatus::Failed;
    }

    std::unique_ptr<Doc> newDoc(fop->releaseDocument());
    if (!newDoc || !newDoc->sprite()) {
      if (error)
        *error = Strings::auto_reload_unsupported();
      return ReloadStatus::Failed;
    }

    std::unique_ptr<doc::Sprite> pendingSprite(newDoc->sprites().front());
    newDoc->sprites().remove(pendingSprite.get());

    const FormatOptionsPtr newFormatOptions = newDoc->formatOptions();
    const bool newReadOnly = newDoc->isReadOnly();

    Tx tx(doc, Strings::commands_ReloadFile(), ModifyDocument);
    cmd::ReloadSprite* reloadCmd = new cmd::ReloadSprite(doc,
                                                        pendingSprite.get(),
                                                        newFormatOptions,
                                                        newReadOnly);
    pendingSprite.release();
    tx(reloadCmd);
    tx.commit();

    doc->markAsSaved();
  }
  catch (const std::exception& e) {
    if (error)
      *error = e.what();
    return ReloadStatus::Failed;
  }

  if (auto statusBar = StatusBar::instance(); ctx && ctx->isUIAvailable() && statusBar) {
    statusBar->setStatusText(2000,
                             Strings::auto_reload_reloaded(base::get_file_name(filename)));
  }

  return ReloadStatus::Reloaded;
}

} // namespace app
