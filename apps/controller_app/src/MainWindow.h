#pragma once

#include <QWidget>
#include <QSlider>
#include <QPlainTextEdit>
#include <QPushButton>

namespace ui {
class StatusStrip;
class ControlPanel;
class TelemetryChart;
class ReadoutStrip;
class LogPanel;
}

/**
 * 主窗口：仅负责界面初始化与各 UI 层组合，不包含具体控件实现。
 * 各层分离：StatusStrip / ControlPanel / TelemetryChart / ReadoutStrip / LogPanel。
 */
class MainWindow : public QWidget {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);

  QPushButton* startButton() const;
  QPushButton* stopButton() const;
  QSlider* targetSlider() const;
  QPlainTextEdit* logEdit() const;

  void updateState(double position, double velocity, double sensorValue);

public slots:
  void appendLog(const QString& text);
  void setStatusText(const QString& text);
  void clearChart();

signals:
  void startClicked();
  void stopClicked();
  void targetChanged(int value);

private:
  ui::StatusStrip* _statusStrip{nullptr};
  ui::ControlPanel* _controlPanel{nullptr};
  ui::TelemetryChart* _telemetryChart{nullptr};
  ui::ReadoutStrip* _readoutStrip{nullptr};
  ui::LogPanel* _logPanel{nullptr};
};
