#include "MainWindow.h"
#include "ui/ControlPanel.h"
#include "ui/LogPanel.h"
#include "ui/ReadoutStrip.h"
#include "ui/StatusStrip.h"
#include "ui/StyleHelper.h"
#include "ui/TelemetryChart.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
  setObjectName("mainContainer");
  setWindowTitle(QStringLiteral("Medical Robot Control Demo"));
  setMinimumWidth(720);
  setStyleSheet(ui::sciFiStylesheet());
  setAutoFillBackground(true);

  auto* root = new QVBoxLayout(this);
  root->setSpacing(12);
  root->setContentsMargins(12, 12, 12, 12);

  _statusStrip = new ui::StatusStrip(this);
  root->addWidget(_statusStrip);

  auto* topRow = new QHBoxLayout();
  topRow->setSpacing(16);
  _controlPanel = new ui::ControlPanel(this);
  _telemetryChart = new ui::TelemetryChart(this);
  topRow->addWidget(_controlPanel);
  topRow->addWidget(_telemetryChart, 1);
  root->addLayout(topRow);

  _readoutStrip = new ui::ReadoutStrip(this);
  root->addWidget(_readoutStrip);

  _logPanel = new ui::LogPanel(this);
  root->addWidget(_logPanel);

  connect(_controlPanel, &ui::ControlPanel::startClicked, this, &MainWindow::startClicked);
  connect(_controlPanel, &ui::ControlPanel::stopClicked, this, &MainWindow::stopClicked);
  connect(_controlPanel, &ui::ControlPanel::targetChanged, this, &MainWindow::targetChanged);
}

QPushButton* MainWindow::startButton() const {
  return _controlPanel ? _controlPanel->startButton() : nullptr;
}

QPushButton* MainWindow::stopButton() const {
  return _controlPanel ? _controlPanel->stopButton() : nullptr;
}

QSlider* MainWindow::targetSlider() const {
  return _controlPanel ? _controlPanel->targetSlider() : nullptr;
}

QPlainTextEdit* MainWindow::logEdit() const {
  return _logPanel ? _logPanel->logEdit() : nullptr;
}

void MainWindow::updateState(double position, double velocity, double sensorValue) {
  if (_readoutStrip) _readoutStrip->updateValues(position, velocity, sensorValue);
  if (_telemetryChart) _telemetryChart->appendSample(position, sensorValue);
}

void MainWindow::appendLog(const QString& text) {
  if (_logPanel) _logPanel->appendLog(text);
}

void MainWindow::setStatusText(const QString& text) {
  if (_statusStrip) _statusStrip->setStatusText(text);
}

void MainWindow::clearChart() {
  if (_telemetryChart) _telemetryChart->clearChart();
}
