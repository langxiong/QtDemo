#include "ControllerApp.h"
#include "QtSubsystem.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMetaObject>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDir>

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

#include "common/config/Config.h"
#include "common/controller/ControllerRuntime.h"
#include "common/log/Log.h"
#include "common/sensor/SensorPipeline.h"
#include "common/status/Models.h"
#include "common/status/StatusSnapshot.h"
#include "DemoController.h"
#include "MainWindow.h"

#include <Poco/Path.h>
#include <chrono>
#include <string>
#include <thread>

static void AddRow(QGridLayout* grid, int row, const QString& name, QLabel*& valueOut, QWidget* parent) {
  auto* nameLbl = new QLabel(name, parent);
  auto* valLbl = new QLabel("-", parent);
  grid->addWidget(nameLbl, row, 0);
  grid->addWidget(valLbl, row, 1);
  valueOut = valLbl;
}

ControllerApp::ControllerApp() {
  setUnixOptions(true);  // Parse --verify-startup (Windows default uses /option)
}

void ControllerApp::initialize(Poco::Util::Application& self) {
  loadConfiguration();
  addSubsystem(new QtSubsystem);
  Application::initialize(self);
  common::log::InitFromConfig(config(), "controller_app");
}

void ControllerApp::defineOptions(Poco::Util::OptionSet& options) {
  Application::defineOptions(options);
  options.addOption(Poco::Util::Option("verify-startup", "v", "Verify startup handshake and exit")
                        .required(false)
                        .repeatable(false)
                        .callback(Poco::Util::OptionCallback<ControllerApp>(this, &ControllerApp::handleOption)));
}

void ControllerApp::handleOption(const std::string& name, const std::string& value) {
  if (name == "verify-startup") {
    _verifyStartup = true;
    return;
  }
  Application::handleOption(name, value);
}

int ControllerApp::main(const std::vector<std::string>& args) {
  common::log::SetThreadName("ui");
  common::log::Info("main", "controller_app starting");

  auto& qtSys = getSubsystem<QtSubsystem>();
  QApplication* qtApp = qtSys.qApplication();
  if (!qtApp) {
    common::log::Error("main", "QtSubsystem failed to create QApplication");
    return Application::EXIT_SOFTWARE;
  }

  std::string appDirPath;
  Poco::Path appPath;
  getApplicationPath(appPath);
  appDirPath = appPath.parent().absolute().toString();

  common::config::Config cfg(config());
  const int sensorRateHz = config().getInt("sensor.rate_hz", 200);
  const int uiRefreshHz = config().getInt("ui.refresh_hz", 30);

  common::sensor::SensorPipeline sensor(common::sensor::SensorPipeline::Params{sensorRateHz});
  sensor.start();

  common::status::StatusStore statusStore;
  common::controller::ControllerRuntime runtime(cfg, sensor, statusStore, appDirPath);
  runtime.start();

  if (_verifyStartup) {
    const int timeoutMs = config().getInt("ipc.ready_timeout_ms", 10000) + 5000;
    const int pollMs = 200;
    int elapsed = 0;
    while (elapsed < timeoutMs) {
      const auto st = statusStore.read();
      if (st.algoHealth == common::status::AlgoHealthState::Healthy) {
        runtime.stop();
        sensor.stop();
        common::log::Info("main", "verify-startup: handshake OK");
        return Application::EXIT_OK;
      }
      if (st.systemState == common::status::SystemState::Degraded && st.lastError != common::status::ErrorCode::Ok) {
        runtime.stop();
        sensor.stop();
        common::log::Error("main", "verify-startup: handshake failed");
        return Application::EXIT_UNAVAILABLE;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
      elapsed += pollMs;
    }
    runtime.stop();
    sensor.stop();
    common::log::Error("main", "verify-startup: timeout waiting for healthy");
    return Application::EXIT_UNAVAILABLE;
  }

  QWidget window;
  window.setWindowTitle(QStringLiteral("Medical Robot Control Demo"));

  auto* root = new QVBoxLayout(&window);

  auto* demoView = new MainWindow(&window);
  root->addWidget(demoView);
  DemoController demoController(demoView);
  const int simIntervalMs = 50;
  demoController.startSimulationTimer(simIntervalMs);

  common::log::SetSink([demoView](common::log::Level level, const std::string& module, const std::string& message) {
    QString line = QString::fromStdString("[" + common::log::LevelToString(level) + "][" + module + "] " + message);
    QMetaObject::invokeMethod(demoView, "appendLog", Qt::QueuedConnection, Q_ARG(QString, line));
  });

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

    if (seriesCmd->count() >= kWindow) {
      seriesCmd->removePoints(0, seriesCmd->count() - (kWindow - 1));
    }
    seriesCmd->append(sampleIndex++, st.lastCommand);

    const int minX = std::max(0, sampleIndex - kWindow);
    axisX->setRange(minX, minX + (kWindow - 1));

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

  const int rc = qtApp->exec();

  demoController.stopSimulationTimer();
  timer.stop();
  runtime.stop();
  sensor.stop();

  common::log::Info("main", "controller_app exiting");
  return rc;
}
