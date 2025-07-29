# BlockHouse Quantitative Developer Trial

This project reconstructs MBP-10 (market by price) data from a stream of MBO (market by order) messages.  The provided `reconstruction` program reads `mbo.csv` and writes an MBP snapshot for every input event.

## Building

```bash
make
```

## Usage

```bash
./reconstruction mbo.csv > my_output.csv
```

The output format matches the columns of `mbp.csv`.  The program combines `T`‑`F`‑`C` sequences into a single trade action and ignores trades with side `N` as specified in the task description.
