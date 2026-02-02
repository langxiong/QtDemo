#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

#include "common/config/Config.h"
#include "common/controller/ControllerRuntime.h"
#include "common/log/Log.h"
#include "common/sensor/SensorPipeline.h"
#include "common/status/StatusSnapshot.h"

static void AddRow(QGridLayout* grid, int row, const QString& name, QLabel*& valueOut, QWidget* parent) {
  auto* nameLbl = new QLabel(name, parent);
  auto* valLbl = new QLabel("-", parent);
  grid->addWidget(nameLbl, row, 0);
  grid->addWidget(valLbl, row, 1);
  valueOut = valLbl;
}

int main(int argc, char* argv[]) {
  common::log::Log::Init("controller_app");
  common::log::Log::SetThreadName("ui");
  common::log::Log::Info("main", "controller_app starting");

  QApplication app(argc, argv);

  common::config::Config cfg;
  if (!cfg.load("config/app.ini")) {
    common::log::Log::Error("config", "failed to load config/app.ini");
  }

  const int sensorRateHz = cfg.getInt("sensor.rate_hz", 200);
  const int uiRefreshHz = cfg.getInt("ui.refresh_hz", 30);

  common::sensor::SensorPipeline sensor(common::sensor::SensorPipeline::Params{sensorRateHz});
  sensor.start();

  common::status::StatusStore statusStore;
  common::controller::ControllerRuntime runtime(cfg, sensor, statusStore);
  runtime.start();

  QWidget window;
  window.setWindowTitle(QStringLiteral("Medical Robot Control Demo"));

  auto* root = new QVBoxLayout(&window);

  // Panels
  auto* grpSignals = new QGroupBox("Signals", &window);
  auto* grpHealth = new QGroupBox("Process/Health", &window);
  auto* grpControl = new QGroupBox("Control/Actuator", &window);

  auto* gridSignals = new QGridLayout(grpSignals);
  auto* gridHealth = new QGridLayout(grpHealth);
  auto* gridControl = new QGridLayout(grpControl);

  QLabel *valRate, *valSeq, *valA, *valB, *valC, *valMiss;
  AddRow(gridSignals, 0, "Sensor rate (Hz):", valRate, grpSignals);
  AddRow(gridSignals, 1, "Sensor seq:", valSeq, grpSignals);
  AddRow(gridSignals, 2, "Value A:", valA, grpSignals);
  AddRow(gridSignals, 3, "Value B:", valB, grpSignals);
  AddRow(gridSignals, 4, "Value C:", valC, grpSignals);
  AddRow(gridSignals, 5, "Missed deadlines:", valMiss, grpSignals);

  QLabel *valState, *valAlgo, *valRtt, *valRestarts;
  AddRow(gridHealth, 0, "System state:", valState, grpHealth);
  AddRow(gridHealth, 1, "Algo health:", valAlgo, grpHealth);
  AddRow(gridHealth, 2, "Heartbeat RTT (ms):", valRtt, grpHealth);
  AddRow(gridHealth, 3, "Algo restarts:", valRestarts, grpHealth);

  QLabel *valCmd, *valPos, *valVel, *valAlgoLat;
  AddRow(gridControl, 0, "Last command:", valCmd, grpControl);
  AddRow(gridControl, 1, "Actuator position:", valPos, grpControl);
  AddRow(gridControl, 2, "Actuator velocity:", valVel, grpControl);
  AddRow(gridControl, 3, "Algo latency (ms):", valAlgoLat, grpControl);

  // Trend plot (Qt Charts)
  auto* seriesCmd = new QLineSeries(&window);
  seriesCmd->setName("cmd");

  auto* chart = new QChart();
  chart->addSeries(seriesCmd);
  chart->legend()->hide();
  chart->setTitle("Control Command Trend");

  auto* axisX = new QValueAxis();
  axisX->setLabelFormat("%d");
  axisX->setTitleText("samples");
  axisX->setRange(0, 299);

  auto* axisY = new QValueAxis();
  axisY->setTitleText("cmd");
  axisY->setRange(-2.0, 2.0);

  chart->addAxis(axisX, Qt::AlignBottom);
  chart->addAxis(axisY, Qt::AlignLeft);
  seriesCmd->attachAxis(axisX);
  seriesCmd->attachAxis(axisY);

  auto* chartView = new QChartView(chart, &window);
  chartView->setRenderHint(QPainter::Antialiasing);
  chartView->setMinimumHeight(220);

  root->addWidget(grpSignals);
  root->addWidget(grpHealth);
  root->addWidget(grpControl);
  root->addWidget(chartView);

  static constexpr int kWindow = 300;
  int sampleIndex = 0;

  QTimer timer;
  QObject::connect(&timer, &QTimer::timeout, [&]() {
    const auto snap = sensor.latest();

    valRate->setText(QString::number(snap.effectiveRateHz, 'f', 1));
    valSeq->setText(QString::number(static_cast<qulonglong>(snap.latest.seq)));
    valA->setText(QString::number(snap.latest.valueA, 'f', 4));
    valB->setText(QString::number(snap.latest.valueB, 'f', 4));
    valC->setText(QString::number(snap.latest.valueC, 'f', 4));
    valMiss->setText(QString::number(static_cast<qulonglong>(snap.missedDeadlines)));

    const auto st = statusStore.read();
    valState->setText(QString::fromLatin1(common::status::ToString(st.systemState)));
    valAlgo->setText(QString::fromLatin1(common::status::ToString(st.algoHealth)));
    valRtt->setText(QString::number(st.heartbeatRttMs, 'f', 2));
    valRestarts->setText(QString::number(static_cast<qulonglong>(st.algoRestarts)));

    valCmd->setText(QString::number(st.lastCommand, 'f', 4));
    valPos->setText(QString::number(st.actuatorPosition, 'f', 4));
    valVel->setText(QString::number(st.actuatorVelocity, 'f', 4));
    valAlgoLat->setText(QString::number(st.algoLatencyMs, 'f', 2));

    // Update trend
    if (seriesCmd->count() >= kWindow) {
      seriesCmd->removePoints(0, seriesCmd->count() - (kWindow - 1));
    }
    seriesCmd->append(sampleIndex++, st.lastCommand);

    const int minX = std::max(0, sampleIndex - kWindow);
    axisX->setRange(minX, minX + (kWindow - 1));

    // Auto-scale Y lightly for demo
    const double y = st.lastCommand;
    if (y < axisY->min() || y > axisY->max()) {
      const double pad = 0.2;
      axisY->setRange(y - 1.0 - pad, y + 1.0 + pad);
    }
  });

  const int intervalMs = (uiRefreshHz > 0) ? (1000 / uiRefreshHz) : 33;
  timer.start(intervalMs);

  window.resize(760, 900);
  window.show();

  const int rc = app.exec();

  timer.stop();
  runtime.stop();
  sensor.stop();

  common::log::Log::Info("main", "controller_app exiting");
  return rc;
}
