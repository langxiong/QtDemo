#include "StatusStrip.h"
#include <QVBoxLayout>

namespace ui {

StatusStrip::StatusStrip(QWidget* parent) : QWidget(parent) {
  auto* lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 4);
  _label = new QLabel(tr("STATUS: STANDBY  |  ROBOT CONTROL CONSOLE"), this);
  _label->setObjectName("statusLabel");
  _label->setAlignment(Qt::AlignCenter);
  lay->addWidget(_label);
}

void StatusStrip::setStatusText(const QString& text) {
  if (_label) _label->setText(text);
}

} // namespace ui
