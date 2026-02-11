#include "TelemetryChart.h"
#include <QChart>
#include <QChartView>
#include <QColor>
#include <QLineSeries>
#include <QValueAxis>
#include <QVBoxLayout>

namespace ui {

TelemetryChart::TelemetryChart(QWidget* parent) : QWidget(parent) {
  auto* lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 0);

  _seriesPosition = new QLineSeries(this);
  _seriesPosition->setName(tr("Position"));
  _seriesSensor = new QLineSeries(this);
  _seriesSensor->setName(tr("Sensor"));

  _chart = new QChart();
  _chart->addSeries(_seriesPosition);
  _chart->addSeries(_seriesSensor);
  _chart->legend()->setVisible(true);
  _chart->legend()->setAlignment(Qt::AlignTop);
  _chart->legend()->setLabelColor(QColor(0x7e, 0xe8, 0xc9));
  _chart->setTitle(tr("TELEMETRY"));
  _chart->setTitleBrush(QBrush(QColor(0x00, 0xd4, 0xaa)));
  _chart->setBackgroundBrush(QBrush(QColor(0x0a, 0x0e, 0x14)));
  _chart->setBackgroundRoundness(4);
  _chart->setBackgroundPen(QPen(QColor(0x1e, 0x3a, 0x2f)));
  _chart->setAnimationOptions(QChart::NoAnimation);

  _axisX = new QValueAxis();
  _axisX->setLabelFormat("%d");
  _axisX->setTitleText(tr("samples"));
  _axisX->setRange(0, kWindow - 1);
  _axisX->setLabelsColor(QColor(0x5a, 0x9b, 0x8a));
  _axisX->setGridLineColor(QColor(0x1e, 0x3a, 0x2f));
  _axisX->setLinePenColor(QColor(0x1e, 0x3a, 0x2f));

  _axisY = new QValueAxis();
  _axisY->setTitleText(tr("value"));
  _axisY->setRange(-1.5, 1.5);
  _axisY->setLabelsColor(QColor(0x5a, 0x9b, 0x8a));
  _axisY->setGridLineColor(QColor(0x1e, 0x3a, 0x2f));
  _axisY->setLinePenColor(QColor(0x1e, 0x3a, 0x2f));

  _chart->addAxis(_axisX, Qt::AlignBottom);
  _chart->addAxis(_axisY, Qt::AlignLeft);
  _seriesPosition->attachAxis(_axisX);
  _seriesPosition->attachAxis(_axisY);
  _seriesSensor->attachAxis(_axisX);
  _seriesSensor->attachAxis(_axisY);

  _seriesPosition->setPen(QPen(QColor(0x00, 0xd4, 0xaa), 2));
  _seriesSensor->setPen(QPen(QColor(0x7e, 0xe8, 0xc9), 1));

  _chartView = new QChartView(_chart, this);
  _chartView->setRenderHint(QPainter::Antialiasing);
  _chartView->setMinimumHeight(240);
  _chartView->setBackgroundBrush(QBrush(QColor(0x0a, 0x0e, 0x14)));
  lay->addWidget(_chartView);
}

void TelemetryChart::appendSample(double position, double sensorValue) {
  const int idx = _sampleIndex++;
  if (_seriesPosition->count() >= kWindow) {
    _seriesPosition->removePoints(0, _seriesPosition->count() - (kWindow - 1));
    _seriesSensor->removePoints(0, _seriesSensor->count() - (kWindow - 1));
  }
  _seriesPosition->append(idx, position);
  _seriesSensor->append(idx, sensorValue);

  const int minX = qMax(0, _sampleIndex - kWindow);
  _axisX->setRange(minX, minX + kWindow - 1);

  const double y = qMax(qAbs(position), qAbs(sensorValue));
  if (y > 0 && (y < _axisY->min() || y > _axisY->max())) {
    const double pad = 0.2;
    _axisY->setRange(-y - pad, y + pad);
  }
}

void TelemetryChart::clearChart() {
  _seriesPosition->clear();
  _seriesSensor->clear();
  _sampleIndex = 0;
  _axisX->setRange(0, kWindow - 1);
}

} // namespace ui
