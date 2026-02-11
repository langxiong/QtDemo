#pragma once

#include <QPlainTextEdit>
#include <QWidget>

namespace ui {

/** 日志层：只读文本框，接收追加的日志行。 */
class LogPanel : public QWidget {
  Q_OBJECT
public:
  explicit LogPanel(QWidget* parent = nullptr);

  QPlainTextEdit* logEdit() const { return _edit; }

public slots:
  void appendLog(const QString& text);

private:
  QPlainTextEdit* _edit{nullptr};
};

} // namespace ui
