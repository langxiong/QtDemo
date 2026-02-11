# Medical Robot Control Demo — Design Flow

## 1. Process & IPC overview

```mermaid
flowchart LR
  subgraph Controller["controller_app"]
    UI[Qt UI / Main]
    RT[ControllerRuntime]
    CL[ControlLoop]
    HB[HeartbeatMonitor]
    PM[AlgoProcessManager]
    SP[SensorPipeline]
    SS[StatusStore]
    UI --> RT
    RT --> PM
    RT --> HB
    RT --> CL
    RT --> SS
    CL --> SP
    CL --> SS
    SP --> SS
    HB --> SS
  end

  subgraph Algo["algo_worker"]
    Main[main loop]
    Srv[IpcServer]
    Main --> Srv
  end

  PM -.->|launch + stdout pipe ready byte| Algo
  RT <-->|TCP: sensor frames, ping/pong, algo results| Srv
```

## 2. Controller process — threads & data flow

```mermaid
flowchart TB
  subgraph Threads["Threads"]
    T_UI[UI thread]
    T_Controller[controller thread]
    T_Sensor[sensor thread]
    T_Heartbeat[heartbeat thread]
    T_Control[control thread]
    T_IPC[IPC receiver thread]
  end

  subgraph Data["Shared data"]
    Channel[DoubleBufferChannel]
    Status[StatusStore]
  end

  T_Sensor -->|write back, publishSwap| Channel
  T_Control -->|readSnapshot| Channel
  T_Controller -->|readSnapshot| Channel
  T_UI -->|read| Status
  T_Controller -->|update| Status
  T_Control -->|update| Status
  T_Heartbeat -->|healthy/unhealthy| T_Controller
  T_IPC -->|pong, algo result| T_Control
  T_Controller -->|ensureConnected, restart| PM[AlgoProcessManager]
```

## 3. Startup & heartbeat / recovery flow

```mermaid
stateDiagram-v2
  [*] --> Starting
  Starting --> Running: connected + healthy
  Starting --> Degraded: connect fail / ready timeout
  Running --> Degraded: N consecutive unhealthy
  Degraded --> Running: restart + connect + healthy
  Degraded --> Degraded: restart limit reached
  Running --> Stopping: stop()
  Degraded --> Stopping: stop()
  Stopping --> [*]
```

## 4. IPC message flow (controller ↔ algo_worker)

```mermaid
sequenceDiagram
  participant C as controller_app
  participant A as algo_worker

  Note over C: startProcess(): launch + pipe
  A->>C: ready byte on stdout pipe
  C->>A: TCP connect
  loop Heartbeat
    C->>A: Ping
    A->>C: Pong (RTT)
  end
  loop Control
    C->>A: SensorFrame
    A->>C: AlgoResult
  end
  Note over C: If N consecutive no-Pong
  C->>C: restart(): kill, backoff, launch, wait ready, connect
```

## 5. Double-buffer channel (sensor → consumers)

```mermaid
flowchart LR
  subgraph Producer["Producer (sensor thread)"]
    W[write to back]
    S[publishSwap]
    W --> S
  end

  subgraph Channel["DoubleBufferChannel"]
    F[front]
    B[back]
  end

  subgraph Consumers["Consumers"]
    CL[ControlLoop]
    RT[ControllerRuntime]
    IPC[IPC send]
  end

  S --> F
  S --> B
  F --> CL
  F --> RT
  F --> IPC
```

You can render these in VS Code (Mermaid extension), on GitHub, or at [mermaid.live](https://mermaid.live).
