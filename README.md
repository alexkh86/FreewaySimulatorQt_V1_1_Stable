# Freeway Simulator Qt 6.11 - V1.1 Stable

This revision consolidates the assertion fixes found during Qt 6.11.2 testing.

Key changes:
- no QCustomPlot refresh during MainWindow construction
- QTimer is heap-owned by MainWindow and uses a fixed 20 ms cadence
- playback uses a simulation-time accumulator; Maximum mode uses bounded batches
- initialization performs only lightweight progress/freeway refresh
- QCPColorMap heatmaps are not created at simulation step 0
- heatmaps contain only completed simulation samples, not future NaNs
- expensive plots refresh every ~60 simulated seconds and on Pause/Step/Leap/end
- synchronous `replot()` is used instead of queued replots

QCustomPlot is not redistributed. Copy `qcustomplot.h` and `qcustomplot.cpp` into `third_party/qcustomplot/`.
