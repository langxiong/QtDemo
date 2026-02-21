/**
 * API client for CEF bridge.
 * In CEF: window.api.notify(method, params) and window.api.call(method, params) return Promise
 * In dev (browser): uses mock when window.api is absent
 */

let callIdCounter = 0;

function nextCallId() {
  callIdCounter += 1;
  return `call-${Date.now()}-${callIdCounter}`;
}

/** Notify: fire-and-forget, no response */
export function notify(method, params = {}) {
  if (window.api?.notify) {
    window.api.notify(method, params);
    return;
  }
  console.warn('[api] window.api.notify not available (CEF not loaded), ignoring:', method);
}

/** Call: returns Promise<{ callId, result }>; result has error_code (0=ok, !=0 error with error msg) */
export function call(method, params = {}) {
  if (window.api?.call) {
    return window.api.call(method, params);
  }
  return mockCall(method, params);
}

function mockCall(method, params) {
  console.warn('[api] window.api.call not available, using mock for:', method);
  return Promise.resolve(mockResponse(method, params));
}

function mockResponse(method, params) {
  const callId = nextCallId();
  if (method === 'readApp') {
    return {
      callId,
      result: { error_code: 0, id: params.id ?? 1, name: 'demo_app', status: 'stopped' },
    };
  }
  if (method === 'readStream') {
    return {
      callId,
      result: {
        error_code: 0,
        streamId: params.streamId ?? 1,
        payload: String(Math.sin(Date.now() / 1000) * 10 + 50),
        timestamp: Date.now(),
      },
    };
  }
  return { callId, result: { error_code: -1, error: `mock: unknown method ${method}` } };
}
