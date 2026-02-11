#include "DemoController.h"
#include "DeviceSimulator.h"
#include "MainWindow.h"

#include <QTimer>
#include <QDateTime>

DemoController::DemoController(MainWindow* view, QObject* parent) : QObject(parent), _view(view) {
  _device = new DeviceSimulator(this);

  connect(_device, &DeviceSimulator::stateUpdated, this, &DemoController::onStateUpdated, Qt::DirectConnection);
  connect(_device, &DeviceSimulator::started, this, &DemoController::onDeviceStarted);
  connect(_device, &DeviceSimulator::stopped, this, &DemoController::onDeviceStopped);

  connect(_view, &MainWindow::startClicked, this, &DemoController::onStartClicked);
  connect(_view, &MainWindow::stopClicked, this, &DemoController::onStopClicked);
  connect(_view, &MainWindow::targetChanged, this, &DemoController::onTargetChanged);

  // 同步滑块初始值为目标，避免 Start 后 target=0 导致直线
  _device->setTarget(_view->targetSlider()->value() / 100.0);
}

DemoController::~DemoController() {
  stopSimulationTimer();
}

void DemoController::startSimulationTimer(int intervalMs) {
  if (_simTimer) return;
  _simTimer = new QTimer(this);
  connect(_simTimer, &QTimer::timeout, this, &DemoController::onSimulationTimeout);
  _lastStepTimeMs = QDateTime::currentMSecsSinceEpoch();
  _simTimer->start(intervalMs);
}

void DemoController::stopSimulationTimer() {
  if (_simTimer) {
    _simTimer->stop();
    _simTimer->deleteLater();
    _simTimer = nullptr;
  }
}

void DemoController::onSimulationTimeout() {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  const double dt = (_lastStepTimeMs > 0) ? ((now - _lastStepTimeMs) / 1000.0) : 0.02;
  _lastStepTimeMs = now;
  if (dt > 0 && dt < 1.0)
    _device->step(dt);
}

void DemoController::onStateUpdated(double position, double velocity, double sensorValue) {
  _view->updateState(position, velocity, sensorValue);
}

void DemoController::onStartClicked() {
  _device->start();
  _view->appendLog(tr("[User] Start clicked"));
}

void DemoController::onStopClicked() {
  _device->stop();
  _view->appendLog(tr("[User] Stop clicked"));
}

void DemoController::onTargetChanged(int value) {
  const double target = value / 100.0;
  _device->setTarget(target);
  _view->appendLog(tr("[User] Target set to %1").arg(target, 0, 'f', 2));
}

void DemoController::onDeviceStarted() {
  _view->setStatusText(tr("STATUS: RUNNING  |  ROBOT CONTROL CONSOLE"));
  _view->appendLog(tr("[System] Device started"));
}

void DemoController::onDeviceStopped() {
  _view->setStatusText(tr("STATUS: STANDBY  |  ROBOT CONTROL CONSOLE"));
  _view->appendLog(tr("[System] Device stopped"));
  _view->clearChart();
}
