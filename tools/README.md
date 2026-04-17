# tools/

Host-side utilities for the STM32 datalogger.

## live_plot.py

Streams lines from the board's UART, parses the `T_INT=` / `T_EXT=` fields
(falls back to the legacy `T=` field), and renders a rolling matplotlib
plot. Press Ctrl+C to save a `capture.csv` with everything seen so far.

### Install

```
python -m venv .venv
# Windows
.venv\Scripts\activate
# macOS / Linux
source .venv/bin/activate

pip install -r requirements.txt
```

### Run

```
python live_plot.py --port COM5
python live_plot.py --port /dev/ttyACM0 --baud 115200 --window 400
```

### Arguments

| Flag       | Default | Description                                   |
|------------|---------|-----------------------------------------------|
| `--port`   | (req.)  | Serial device, e.g. `COM5`, `/dev/ttyACM0`.   |
| `--baud`   | 115200  | Must match the firmware's UART baud.          |
| `--window` | 200     | Rolling point count shown on the plot.        |

### Output

On Ctrl+C the script writes `capture.csv` in the current directory with
columns: `t_s,t_internal_c,t_external_c`.
