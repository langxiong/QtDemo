#pragma once

#include <QString>

namespace ui {

/** 科幻风全局样式，由 MainWindow 在初始化时应用。 */
inline QString sciFiStylesheet() {
  return QStringLiteral(R"(
  MainWindow, QWidget#mainContainer {
    background-color: #0a0e14;
  }
  QGroupBox {
    font-weight: bold;
    font-size: 10px;
    color: #7ee8c9;
    border: 1px solid #1e3a2f;
    border-radius: 4px;
    margin-top: 10px;
    padding: 12px 10px 6px 12px;
    background-color: #0d1219;
  }
  QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 10px;
    padding: 0 6px;
    color: #00d4aa;
  }
  QPushButton {
    background-color: #132a22;
    color: #7ee8c9;
    border: 1px solid #1e3a2f;
    border-radius: 4px;
    padding: 8px 16px;
    font-weight: bold;
    min-width: 72px;
  }
  QPushButton:hover {
    background-color: #1e3a2f;
    border-color: #00d4aa;
  }
  QPushButton:pressed { background-color: #0d1f18; }
  QPushButton#startBtn {
    color: #00d4aa;
    border-color: #00d4aa;
  }
  QPushButton#startBtn:hover { background-color: #0d2a22; }
  QPushButton#stopBtn {
    color: #ff6b6b;
    border-color: #4a2020;
    background-color: #2a1515;
  }
  QPushButton#stopBtn:hover {
    border-color: #ff6b6b;
    background-color: #3a2020;
  }
  QSlider::groove:horizontal {
    height: 6px;
    background: #1a2332;
    border-radius: 3px;
  }
  QSlider::handle:horizontal {
    width: 16px;
    margin: -5px 0;
    background: #00d4aa;
    border-radius: 8px;
  }
  QSlider::handle:horizontal:hover { background: #33e0b8; }
  QSlider::sub-page:horizontal { background: #1e3a2f; border-radius: 3px; }
  QPlainTextEdit#logBox {
    background-color: #05080c;
    color: #7ee8c9;
    border: 1px solid #1e3a2f;
    border-radius: 4px;
    padding: 8px;
    font-family: Consolas, 'Courier New', monospace;
    font-size: 11px;
  }
  QLabel#statusLabel {
    color: #7ee8c9;
    font-size: 11px;
    font-family: Consolas, monospace;
  }
  QFrame#readoutFrame {
    background-color: #0d1219;
    border: 1px solid #1e3a2f;
    border-radius: 4px;
    padding: 8px 12px;
  }
  QLabel#readoutTitle {
    color: #5a9b8a;
    font-size: 9px;
    font-weight: bold;
    letter-spacing: 1px;
  }
  QLabel#readoutValue {
    color: #00d4aa;
    font-size: 13px;
    font-family: Consolas, 'Courier New', monospace;
    font-weight: bold;
  }
)");
}

} // namespace ui
