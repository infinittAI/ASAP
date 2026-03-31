#include "ShortcutManager.h"
#include <QSettings>

QHash<QString, QKeySequence> ShortcutManager::_cache;
QList<ShortcutEntry> ShortcutManager::_registry;

QString ShortcutManager::settingsKey(const QString& id) {
  return "Shortcuts/" + id;
}

void ShortcutManager::ensureRegistry() {
  if (!_registry.isEmpty()) return;

  _registry = {
    // Annotation Plugin (keyPressEvent)
    {"annotation_toggle_visibility",  "Toggle annotations",       "Annotation",     QKeySequence("H")},
    {"annotation_delete_selected",    "Delete selected",          "Annotation",     QKeySequence("E")},
    {"annotation_change_color",       "Change color",             "Annotation",     QKeySequence("C")},

    // Annotation Tool (keyPressEvent)
    {"annotation_cancel",             "Cancel annotation",        "Drawing",        QKeySequence("Esc")},
    {"annotation_remove_last_point",  "Remove last point",        "Drawing",        QKeySequence("Del")},
    {"annotation_cancel_entire",      "Cancel entire (shift)",    "Drawing",        QKeySequence("Shift+Del")},

    // Tool Switching (QAction)
    {"tool_dotannotation",            "Dot Annotation Tool",      "Tool Switching", QKeySequence("D")},
    {"tool_polyannotation",           "Poly Annotation Tool",     "Tool Switching", QKeySequence("P")},
    {"tool_splineannotation",         "Spline Annotation Tool",   "Tool Switching", QKeySequence("S")},
    {"tool_rectangleannotation",      "Rectangle Annotation Tool","Tool Switching", QKeySequence("R")},
    {"tool_measurementannotation",    "Measurement Tool",         "Tool Switching", QKeySequence("M")},
    {"tool_pointsetannotation",       "PointSet Tool",            "Tool Switching", QKeySequence("I")},
    {"tool_zoom",                     "Zoom Tool",                "Tool Switching", QKeySequence("Z")},
    {"tool_pan",                      "Pan Tool",                 "Tool Switching", QKeySequence("X")},

    // Window
    {"window_open_file",              "Open file",                "Window",         QKeySequence("Ctrl+O")},
    {"window_close_file",             "Close file",               "Window",         QKeySequence("Ctrl+C")},
    {"window_show_shortcuts",         "Show shortcuts",           "Window",         QKeySequence("F1")},

    // Tree Widget
    {"tree_delete_item",              "Delete tree item",         "Tree",           QKeySequence("Del")},
  };
}

void ShortcutManager::ensureCache() {
  ensureRegistry();
  if (!_cache.isEmpty()) return;

  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP");
  for (const auto& entry : _registry) {
    QString val = settings.value(settingsKey(entry.id)).toString();
    if (val.isEmpty()) {
      _cache[entry.id] = entry.defaultSeq;
    } else {
      _cache[entry.id] = QKeySequence(val);
    }
  }
}

QKeySequence ShortcutManager::getShortcut(const QString& id, const QString& defaultSeq) {
  ensureCache();
  if (_cache.contains(id)) {
    return _cache[id];
  }
  return QKeySequence(defaultSeq);
}

void ShortcutManager::setShortcut(const QString& id, const QKeySequence& seq) {
  ensureCache();
  _cache[id] = seq;
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP");
  if (seq.isEmpty()) {
    settings.remove(settingsKey(id));
  } else {
    settings.setValue(settingsKey(id), seq.toString());
  }
}

void ShortcutManager::removeShortcut(const QString& id) {
  ensureCache();
  ensureRegistry();
  for (const auto& entry : _registry) {
    if (entry.id == id) {
      _cache[id] = entry.defaultSeq;
      break;
    }
  }
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP");
  settings.remove(settingsKey(id));
}

bool ShortcutManager::matchesKeyEvent(const QKeySequence& seq, int key, Qt::KeyboardModifiers modifiers) {
  if (seq.count() != 1) return false;
  QKeyCombination combo = seq[0];
  return combo.key() == static_cast<Qt::Key>(key) && combo.keyboardModifiers() == modifiers;
}

QStringList ShortcutManager::findConflicts(const QString& excludeId, const QKeySequence& seq) {
  ensureCache();
  QStringList conflicts;
  if (seq.isEmpty()) return conflicts;
  for (auto it = _cache.constBegin(); it != _cache.constEnd(); ++it) {
    if (it.key() == excludeId) continue;
    if (it.value() == seq) {
      conflicts.append(it.key());
    }
  }
  return conflicts;
}

QList<ShortcutEntry> ShortcutManager::getAllShortcuts() {
  ensureCache();
  ensureRegistry();
  QList<ShortcutEntry> result;
  for (const auto& entry : _registry) {
    ShortcutEntry e = entry;
    // currentSeq comes from cache (which includes user overrides)
    result.append(e);
  }
  return result;
}

void ShortcutManager::resetToDefaults() {
  ensureRegistry();
  _cache.clear();
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP");
  settings.beginGroup("Shortcuts");
  settings.remove("");
  settings.endGroup();
}
