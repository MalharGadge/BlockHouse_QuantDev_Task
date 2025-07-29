This repository reconstructs MBP-10 order book data from the provided MBO sample.

Build:
  make

Run:
  ./reconstruction mbo.csv > output.csv

The algorithm maintains a limit order book and outputs a snapshot after every
input row. Trade, fill and cancel sequences are collapsed into a single trade
update as described in the task PDF.
