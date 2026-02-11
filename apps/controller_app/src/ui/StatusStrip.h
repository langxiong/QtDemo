#pragma once

#include <QLabel>
#include <QWidget>

namespace ui {

/** 顶部状态条：显示 STATUS: STANDBY / RUNNING 等。 */
class StatusStrip : public QWidget {
  Q_OBJECT
public:
  explicit StatusStrip(QWidget* parent = nullptr);

  QLabel* statusLabel() const { return _label; }

public slots:
  void setStatusText(const QString& text);

private:
  QLabel* _label{nullptr};
};

} // namespace ui
