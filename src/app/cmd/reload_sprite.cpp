// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/reload_sprite.h"

#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/ui/doc_view.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

ReloadSprite::ReloadSprite(Doc* doc,
                           doc::Sprite* newSprite,
                           const FormatOptionsPtr& newFormatOptions,
                           const bool newReadOnly)
  : m_doc(doc)
  , m_newSprite(newSprite)
  , m_ownedSprite(newSprite)
  , m_newFormatOptions(newFormatOptions)
  , m_newReadOnly(newReadOnly)
{
}

ReloadSprite::~ReloadSprite()
{
  delete m_ownedSprite;
}

void ReloadSprite::onExecute()
{
  m_oldSprite = m_doc->sprite();
  m_oldFormatOptions = m_doc->formatOptions();
  m_oldReadOnly = m_doc->isReadOnly();

  replaceSprite(m_newSprite, m_newFormatOptions, m_newReadOnly);
  m_ownedSprite = m_oldSprite;
}

void ReloadSprite::onUndo()
{
  replaceSprite(m_oldSprite, m_oldFormatOptions, m_oldReadOnly);
  m_ownedSprite = m_newSprite;
}

void ReloadSprite::onRedo()
{
  replaceSprite(m_newSprite, m_newFormatOptions, m_newReadOnly);
  m_ownedSprite = m_oldSprite;
}

void ReloadSprite::replaceSprite(doc::Sprite* sprite,
                                 const FormatOptionsPtr& formatOptions,
                                 const bool readOnly)
{
  if (m_doc->sprite() == sprite)
    return;

  if (m_doc->sprite())
    m_doc->sprites().remove(m_doc->sprite());
  m_doc->sprites().add(sprite);

  m_doc->setFormatOptions(formatOptions);
  if (readOnly)
    m_doc->markAsReadOnly();
  else
    m_doc->removeReadOnlyMark();

  Mask emptyMask;
  m_doc->setMask(&emptyMask);
  m_doc->setExtraCel(nullptr);
  m_doc->destroyMaskBoundaries();
  m_doc->resetTransformation();
  m_doc->updateColorSpaceFromSprite();
}

void ReloadSprite::onFireNotifications()
{
  reload_document_editors(m_doc);

  m_doc->notifyGeneralUpdate();
  m_doc->notifyColorSpaceChanged();
  m_doc->notifyPaletteChanged();
}

std::string ReloadSprite::onLabel() const
{
  return Strings::commands_ReloadFile();
}

size_t ReloadSprite::onMemSize() const
{
  size_t size = sizeof(*this);
  if (m_oldSprite)
    size += m_oldSprite->getMemSize();
  if (m_newSprite)
    size += m_newSprite->getMemSize();
  return size;
}

} // namespace cmd
} // namespace app
