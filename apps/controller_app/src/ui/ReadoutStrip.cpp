#include "ReadoutStrip.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QVBoxLayout>

namespace ui {

static QWidget* makeReadoutFrame(const QString& title, QLabel*& valueOut, QWidget* parent) {
  auto* f = new QFrame(parent);
  f->setObjectName("readoutFrame");
  auto* lay = new QVBoxLayout(f);
  lay->setContentsMargins(8, 6, 8, 6);
  auto* titleLbl = new QLabel(title, f);
  titleLbl->setObjectName("readoutTitle");
  auto* valLbl = new QLabel(QObject::tr("—"), f);
  valLbl->setObjectName("readoutValue");
  lay->addWidget(titleLbl);
  lay->addWidget(valLbl);
  valueOut = valLbl;
  return f;
}

ReadoutStrip::ReadoutStrip(QWidget* parent) : QWidget(parent) {
  auto* row = new QHBoxLayout(this);
  row->setSpacing(12);

  row->addWidget(makeReadoutFrame(tr("POSITION"), _labelPosition, this));
  row->addWidget(makeReadoutFrame(tr("VELOCITY"), _labelVelocity, this));
  row->addWidget(makeReadoutFrame(tr("SENSOR"), _labelSensor, this));
  row->addStretch();
}

void ReadoutStrip::updateValues(double position, double velocity, double sensorValue) {
  if (_labelPosition) _labelPosition->setText(QString::number(position, 'f', 4));
  if (_labelVelocity) _labelVelocity->setText(QString::number(velocity, 'f', 4));
  if (_labelSensor) _labelSensor->setText(QString::number(sensorValue, 'f', 4));
}

} // namespace ui
