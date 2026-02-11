#pragma once

#include <QChartView>
#include <QWidget>

class QChart;
class QLineSeries;
class QValueAxis;

namespace ui {

/** 遥测图表层：Position + Sensor 曲线，深色主题。 */
class TelemetryChart : public QWidget {
  Q_OBJECT
public:
  explicit TelemetryChart(QWidget* parent = nullptr);

  /** 追加一帧数据并刷新曲线。 */
  void appendSample(double position, double sensorValue);
  /** 清空曲线（停止时）。 */
  void clearChart();

private:
  QChart* _chart{nullptr};
  QChartView* _chartView{nullptr};
  QLineSeries* _seriesPosition{nullptr};
  QLineSeries* _seriesSensor{nullptr};
  QValueAxis* _axisX{nullptr};
  QValueAxis* _axisY{nullptr};
  int _sampleIndex{0};
  static constexpr int kWindow = 300;
};

} // namespace ui
