#pragma once

#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

namespace ui {

/** 控制层：启停按钮 + 目标滑块，仅发信号不执行业务。 */
class ControlPanel : public QGroupBox {
  Q_OBJECT
public:
  explicit ControlPanel(QWidget* parent = nullptr);

  QPushButton* startButton() const { return _startBtn; }
  QPushButton* stopButton() const { return _stopBtn; }
  QSlider* targetSlider() const { return _targetSlider; }

signals:
  void startClicked();
  void stopClicked();
  void targetChanged(int value);

private:
  QPushButton* _startBtn{nullptr};
  QPushButton* _stopBtn{nullptr};
  QSlider* _targetSlider{nullptr};
};

} // namespace ui
