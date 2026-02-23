#!/usr/bin/env python3
"""
Fetch latest CEF (Chromium Embedded Framework) prebuilt from cef-builds.spotifycdn.com.
Extracts to demo/prebuilt/cef/ and prints MRCD_CEF_ROOT for CMake.

Usage:
  python demo/scripts/fetch_cef.py
  # From project root (parent of demo/)

CEF prebuilt is Release-only; libcef_dll_wrapper builds Debug/Release with main project.
"""
from __future__ import print_function

import json
import os
import sys
import tarfile
import tempfile
import urllib.request

CEF_INDEX_URL = "https://cef-builds.spotifycdn.com/index.json"
CEF_CDN_BASE = "https://cef-builds.spotifycdn.com/"


def get_script_dir():
    return os.path.dirname(os.path.abspath(__file__))


def get_prebuilt_dir():
    """demo/prebuilt/cef"""
    script_dir = get_script_dir()
    demo_dir = os.path.dirname(script_dir)  # demo/
    return os.path.join(demo_dir, "prebuilt", "cef")


def fetch_index():
    print("Fetching CEF index...")
    req = urllib.request.Request(CEF_INDEX_URL)
    with urllib.request.urlopen(req, timeout=30) as resp:
        return json.loads(resp.read().decode())


def get_latest_windows64_url(data):
    versions = data.get("windows64", {}).get("versions", [])
    for v in versions:
        if v.get("channel") == "stable":
            for f in v.get("files", []):
                if f.get("type") == "standard":
                    return CEF_CDN_BASE + f["name"], v.get("cef_version", "unknown")
    raise RuntimeError("No stable windows64 standard build found in index")


def download_and_extract(url, dest_dir):
    os.makedirs(dest_dir, exist_ok=True)
    fname = url.split("/")[-1]
    tmp_path = os.path.join(tempfile.gettempdir(), fname)

    print("Downloading {} (~150-200MB)...".format(fname))
    urllib.request.urlretrieve(url, tmp_path, reporthook=_progress)

    print("\nExtracting to {}...".format(dest_dir))
    with tarfile.open(tmp_path, "r:bz2") as tf:
        tf.extractall(dest_dir)

    try:
        os.remove(tmp_path)
    except OSError:
        pass

    # Find extracted dir: cef_binary_*_windows64
    entries = os.listdir(dest_dir)
    cef_dirs = [e for e in entries if e.startswith("cef_binary_") and e.endswith("_windows64")]
    if not cef_dirs:
        raise RuntimeError("Extracted archive did not create cef_binary_*_windows64 directory")
    return os.path.join(dest_dir, cef_dirs[0])


def _progress(block_num, block_size, total_size):
    if total_size <= 0:
        return
    done = block_num * block_size
    pct = min(100, 100 * done / total_size)
    mb = done / (1024 * 1024)
    total_mb = total_size / (1024 * 1024)
    sys.stderr.write("\r  {:.1f} MB / {:.1f} MB ({:.1f}%)".format(mb, total_mb, pct))
    sys.stderr.flush()


def main():
    prebuilt_dir = get_prebuilt_dir()
    print("CEF prebuilt destination: {}".format(prebuilt_dir))

    data = fetch_index()
    url, version = get_latest_windows64_url(data)
    print("Latest stable: {}".format(version))

    cef_root = download_and_extract(url, prebuilt_dir)

    print("\nDone.")
    print("MRCD_CEF_ROOT={}".format(os.path.abspath(cef_root)))
    print("\nConfigure with: -DMRCD_CEF_ROOT=\"{}\"".format(os.path.abspath(cef_root)))
    print("Or ensure prebuilt/cef exists; CMake will auto-detect.")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("Error: {}".format(e), file=sys.stderr)
        sys.exit(1)
