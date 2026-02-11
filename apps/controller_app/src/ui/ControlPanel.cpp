#include "ControlPanel.h"
#include <QGridLayout>
#include <QLabel>

namespace ui {

ControlPanel::ControlPanel(QWidget* parent) : QGroupBox(tr("CONTROL"), parent) {
  setMaximumWidth(260);
  auto* grid = new QGridLayout(this);

  _startBtn = new QPushButton(tr("START"), this);
  _startBtn->setObjectName("startBtn");
  _stopBtn = new QPushButton(tr("STOP"), this);
  _stopBtn->setObjectName("stopBtn");
  _targetSlider = new QSlider(Qt::Horizontal, this);
  _targetSlider->setRange(-100, 100);
  _targetSlider->setValue(50);

  grid->addWidget(new QLabel(tr("TARGET"), this), 0, 0);
  grid->addWidget(_targetSlider, 1, 0);
  grid->addWidget(_startBtn, 2, 0);
  grid->addWidget(_stopBtn, 3, 0);

  connect(_startBtn, &QPushButton::clicked, this, &ControlPanel::startClicked);
  connect(_stopBtn, &QPushButton::clicked, this, &ControlPanel::stopClicked);
  connect(_targetSlider, &QSlider::valueChanged, this, &ControlPanel::targetChanged);
}

} // namespace ui
