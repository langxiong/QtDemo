#include "LogPanel.h"
#include <QVBoxLayout>
#include <QScrollBar>

namespace ui {

LogPanel::LogPanel(QWidget* parent) : QWidget(parent) {
  auto* lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 0);
  _edit = new QPlainTextEdit(this);
  _edit->setObjectName("logBox");
  _edit->setReadOnly(true);
  _edit->setMinimumHeight(100);
  _edit->setPlaceholderText(tr("> System log..."));
  lay->addWidget(_edit);
}

void LogPanel::appendLog(const QString& text) {
  _edit->appendPlainText(text);
  _edit->verticalScrollBar()->setValue(_edit->verticalScrollBar()->maximum());
}

} // namespace ui
