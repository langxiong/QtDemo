#include "ControllerApp.h"
#include "ControllerRuntimeDds.h"
#include "DeviceSimulator.h"
#include "IpcServer.h"

#include "common/config/ConfigPoco.h"
#include "common/log/Log.h"
#include "common/sensor/SensorPipeline.h"

#include <Poco/Environment.h>
#include <Poco/Path.h>
#include <Poco/Process.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)
#include "UISubsystem.hpp"
#endif

ControllerApp::ControllerApp() {
  setUnixOptions(true);
}

ControllerApp::~ControllerApp() = default;

void ControllerApp::initialize(Poco::Util::Application& self) {
  loadConfiguration();
  Application::initialize(self);
  common::log::InitFromConfig(common::config::WrapPocoConfig(config()), "controller_app");
}

void ControllerApp::defineOptions(Poco::Util::OptionSet& options) {
  Application::defineOptions(options);
  options.addOption(
      Poco::Util::Option("verify-startup", "v", "Verify startup handshake and exit")
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
  (void)args;
  common::log::SetThreadName("main");

  std::string appDirPath;
  Poco::Path appPath;
  getApplicationPath(appPath);
  appDirPath = appPath.parent().absolute().toString();

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)
  common::log::Info("main", "controller_app starting (GUI mode)");
#else
  common::log::Info("main", "controller_app starting (headless)");
#endif

  auto cfg = common::config::WrapPocoConfig(config());
  const int sensorRateHz = config().getInt("sensor.rate_hz", 200);
  const int simIntervalMs = 50;

  common::sensor::SensorPipeline sensor(common::sensor::SensorPipeline::Params{sensorRateHz});
  sensor.start();

  common::status::StatusStore statusStore;
  controller_app::ControllerRuntimeDds runtime(cfg, sensor, statusStore, appDirPath);
  runtime.start();

  DeviceSimulator device;
  std::atomic<bool> simRunning{true};
  std::thread simThread([&device, &simRunning, simIntervalMs]() {
    auto lastStep = std::chrono::steady_clock::now();
    while (simRunning) {
      auto now = std::chrono::steady_clock::now();
      double dt = std::chrono::duration<double>(now - lastStep).count();
      lastStep = now;
      if (dt > 0 && dt < 1.0) device.step(dt);
      std::this_thread::sleep_for(std::chrono::milliseconds(simIntervalMs));
    }
  });

  controller_app::IpcServer ipcServer(cfg, device, statusStore, sensor);
  ipcServer.start();

  if (_verifyStartup) {
    const int timeoutMs = config().getInt("ipc.ready_timeout_ms", 10000) + 5000;
    const int pollMs = 200;
    int elapsed = 0;
    while (elapsed < timeoutMs) {
      const auto st = statusStore.read();
      if (st.algoHealth == common::status::AlgoHealthState::Healthy) {
        ipcServer.stop();
        simRunning = false;
        if (simThread.joinable()) simThread.join();
        runtime.stop();
        sensor.stop();
        common::log::Info("main", "verify-startup: handshake OK");
        return Application::EXIT_OK;
      }
      if (st.systemState == common::status::SystemState::Degraded &&
          st.lastError != common::status::ErrorCode::Ok) {
        ipcServer.stop();
        simRunning = false;
        if (simThread.joinable()) simThread.join();
        runtime.stop();
        sensor.stop();
        common::log::Error("main", "verify-startup: handshake failed");
        return Application::EXIT_UNAVAILABLE;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
      elapsed += pollMs;
    }
    ipcServer.stop();
    simRunning = false;
    if (simThread.joinable()) simThread.join();
    runtime.stop();
    sensor.stop();
    common::log::Error("main", "verify-startup: timeout waiting for healthy");
    return Application::EXIT_UNAVAILABLE;
  }

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)
  _ui = std::make_unique<controller_app::UISubsystem>();
  if (_ui->create(800, 600, L"Medical Robot Control Demo")) {
    _ui->show();
    void* clientHwnd = _ui->getClientHwnd();
    if (clientHwnd) {
      Poco::Path cefHostPath(appDirPath);
      cefHostPath.append("cef_host.exe");
      std::string cefHostExe = cefHostPath.absolute().toString();
      std::string hwndArg = "--parent-hwnd=" + std::to_string(reinterpret_cast<uintptr_t>(clientHwnd));
      Poco::Process::Args launchArgs;
      launchArgs.push_back(hwndArg);
      int apiPort = config().getInt("ipc.api_port", 9123);
      std::string ipcAddr = "127.0.0.1:" + std::to_string(apiPort);
      Poco::Environment::set("MRCD_CONTROLLER_IPC_ADDR", ipcAddr);
      try {
        Poco::ProcessHandle ph = Poco::Process::launch(cefHostExe, launchArgs);
        common::log::Info("main", "launched cef_host with parent-hwnd, IPC=" + ipcAddr);
      } catch (const std::exception& e) {
        common::log::Error("main", std::string("failed to launch cef_host: ") + e.what());
      }
    }
    common::log::Info("main", "controller_app running (GUI). Close window to exit.");
    _ui->runMessageLoop();
  } else {
    common::log::Error("main", "UISubsystem create failed; running headless");
    common::log::Info("main", "controller_app running (IPC server active). Press Ctrl+C to exit.");
    waitForTerminationRequest();
  }
#else
  common::log::Info("main", "controller_app running (IPC server active). Press Ctrl+C to exit.");
  waitForTerminationRequest();
#endif

  ipcServer.stop();
  simRunning = false;
  if (simThread.joinable()) simThread.join();
  runtime.stop();
  sensor.stop();
#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)
  if (_ui) _ui->close();
#endif
  common::log::Info("main", "controller_app exiting");
  return Application::EXIT_OK;
}
