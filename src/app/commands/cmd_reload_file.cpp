// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/auto_reload.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/reload_file.h"
#include "base/fs.h"
#include "ui/alert.h"

#include <algorithm>

namespace app {

class ReloadFileCommand : public Command {
public:
  ReloadFileCommand() : Command(CommandId::ReloadFile()) {}

protected:
  bool onEnabled(Context* ctx) override
  {
    Doc* doc = ctx->activeDocument();
    return (doc && doc->isAssociatedToFile());
  }

  void onExecute(Context* ctx) override
  {
    Doc* doc = ctx->activeDocument();
    if (!doc)
      return;

    std::string error;
    const ReloadStatus status = reload_document(ctx, doc, true, &error);

    switch (status) {
      case ReloadStatus::Reloaded:
        if (AutoReload* autoReload = AutoReload::instance())
          autoReload->resetDoc(doc);
        break;

      case ReloadStatus::Failed:
        if (ctx->isUIAvailable()) {
          std::string details = error;
          std::replace(details.begin(), details.end(), '\n', ' ');
          std::replace(details.begin(), details.end(), '\r', ' ');

          ui::Alert alert;
          alert.setTitle(Strings::auto_reload_title());
          alert.addLabel(Strings::auto_reload_failed(base::get_file_name(doc->filename())),
                         ui::LEFT);
          if (!details.empty())
            alert.addLabel(details, ui::LEFT);
          alert.addButton(Strings::general_ok());
          alert.show();
        }
        break;

      case ReloadStatus::Canceled:
        break;
    }
  }
};

Command* CommandFactory::createReloadFileCommand()
{
  return new ReloadFileCommand;
}

} // namespace app
