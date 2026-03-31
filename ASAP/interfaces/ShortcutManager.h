#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QString>
#include <QKeySequence>
#include <QList>
#include <QStringList>
#include <QHash>

struct ShortcutEntry {
  QString id;
  QString displayName;
  QString group;
  QKeySequence defaultSeq;
};

class ShortcutManager {
public:
  static QKeySequence getShortcut(const QString& id, const QString& defaultSeq);
  static void setShortcut(const QString& id, const QKeySequence& seq);
  static void removeShortcut(const QString& id);

  static bool matchesKeyEvent(const QKeySequence& seq, int key, Qt::KeyboardModifiers modifiers);

  static QStringList findConflicts(const QString& excludeId, const QKeySequence& seq);
  static QList<ShortcutEntry> getAllShortcuts();
  static void resetToDefaults();

private:
  static QHash<QString, QKeySequence> _cache;
  static QList<ShortcutEntry> _registry;
  static void ensureRegistry();
  static void ensureCache();
  static QString settingsKey(const QString& id);
};

#endif
