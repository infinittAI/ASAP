#include "YellowPolyAnnotationTool.h"
#include "AnnotationWorkstationExtensionPlugin.h"
#include <QAction>
#include <QApplication>
#include <QStyleHints>
#include "interfaces/ShortcutManager.h"

YellowPolyAnnotationTool::YellowPolyAnnotationTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer)
  : PolyAnnotationTool(annotationPlugin, viewer)
{
}

std::string YellowPolyAnnotationTool::name() {
  return std::string("yellowpolyannotation");
}

QAction* YellowPolyAnnotationTool::getToolButton() {
  if (!_button) {
    _button = new QAction("&YellowPolyAnnotation", this);
    _button->setObjectName(QString::fromStdString(name()));
    if (QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
      _button->setIcon(QIcon(QPixmap(":/AnnotationWorkstationExtensionPlugin_icons/poly_dark.png")));
    }
    else {
      _button->setIcon(QIcon(QPixmap(":/AnnotationWorkstationExtensionPlugin_icons/poly.png")));
    }
    _button->setShortcut(ShortcutManager::getShortcut("tool_yellowpolyannotation", "B"));
  }
  return _button;
}
