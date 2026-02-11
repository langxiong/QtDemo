#pragma once

#include <QLabel>
#include <QWidget>

namespace ui {

/** 读数值条：POSITION / VELOCITY / SENSOR 三块，仅展示数据。 */
class ReadoutStrip : public QWidget {
  Q_OBJECT
public:
  explicit ReadoutStrip(QWidget* parent = nullptr);

  /** 更新三路数值显示。 */
  void updateValues(double position, double velocity, double sensorValue);

private:
  QLabel* _labelPosition{nullptr};
  QLabel* _labelVelocity{nullptr};
  QLabel* _labelSensor{nullptr};
};

} // namespace ui
