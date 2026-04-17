#!/usr/bin/env python3
"""Live plot of STM32 UART temperature stream.

Parses lines like:
    TEMP_RAW=942 VREF_RAW=1526  T_INT=27.34 C T=27.34 C T_EXT=24.18 C
Falls back to legacy `T=xx.xx C` if T_INT is absent.
"""
import argparse
import csv
import re
import signal
import sys
import time
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import serial


RE_T_INT = re.compile(r"T_INT=(-?\d+(?:\.\d+)?)")
RE_T_EXT = re.compile(r"T_EXT=(-?\d+(?:\.\d+)?)")
RE_T_FB  = re.compile(r"(?<![A-Z_])T=(-?\d+(?:\.\d+)?)")


def parse_args():
    p = argparse.ArgumentParser(description="Live plot of STM32 temperature stream.")
    p.add_argument("--port", required=True, help="Serial port (e.g. COM5 or /dev/ttyACM0)")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--window", type=int, default=200, help="Rolling window size")
    return p.parse_args()


def main():
    args = parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.1)

    ts_buf   = deque(maxlen=args.window)
    int_buf  = deque(maxlen=args.window)
    ext_buf  = deque(maxlen=args.window)
    captured = []

    t0 = time.time()

    fig, ax = plt.subplots()
    line_int, = ax.plot([], [], label="T_INT (C)")
    line_ext, = ax.plot([], [], label="T_EXT (C)")
    ax.set_xlabel("time (s)")
    ax.set_ylabel("temperature (C)")
    ax.set_title("STM32 temperature stream")
    ax.legend(loc="upper right")
    ax.grid(True)

    def read_line():
        try:
            raw = ser.readline()
        except serial.SerialException:
            return None
        if not raw:
            return None
        try:
            return raw.decode("utf-8", errors="replace").strip()
        except UnicodeDecodeError:
            return None

    def update(_frame):
        for _ in range(64):
            s = read_line()
            if not s:
                break
            m_int = RE_T_INT.search(s)
            m_ext = RE_T_EXT.search(s)
            m_fb  = RE_T_FB.search(s)

            t_int = float(m_int.group(1)) if m_int else (float(m_fb.group(1)) if m_fb else None)
            t_ext = float(m_ext.group(1)) if m_ext else None
            if t_int is None and t_ext is None:
                continue

            now = time.time() - t0
            ts_buf.append(now)
            int_buf.append(t_int if t_int is not None else float("nan"))
            ext_buf.append(t_ext if t_ext is not None else float("nan"))
            captured.append((now, t_int, t_ext))

        if ts_buf:
            line_int.set_data(list(ts_buf), list(int_buf))
            line_ext.set_data(list(ts_buf), list(ext_buf))
            ax.relim()
            ax.autoscale_view()
        return line_int, line_ext

    def save_csv(*_):
        path = "capture.csv"
        with open(path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["t_s", "t_internal_c", "t_external_c"])
            for row in captured:
                w.writerow([f"{row[0]:.3f}",
                            "" if row[1] is None else f"{row[1]:.2f}",
                            "" if row[2] is None else f"{row[2]:.2f}"])
        print(f"\nSaved {len(captured)} rows to {path}")
        try:
            ser.close()
        except Exception:
            pass
        sys.exit(0)

    signal.signal(signal.SIGINT, save_csv)

    _ani = animation.FuncAnimation(fig, update, interval=200, cache_frame_data=False)
    plt.show()
    save_csv()


if __name__ == "__main__":
    main()
