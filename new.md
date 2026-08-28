
## Commands (training-data collection)
```powershell
make clean
make
.\flash_and_monitor.ps1
```

## Commands (ML classification test)
```powershell
make clean
make test
.\flash_and_monitor.ps1 -BinName gas_monitor_test.bin
```

If the flash and monitor script doesn't work and the terminal freezes after running the script then unplug your STM32 microcontroller and replug it back in.


## Full Steps

```powershell
cd folder/projectfolder
```

1. **Build**
```powershell
   make          # training firmware -> gas_monitor.bin
   make test     # ML test firmware  -> gas_monitor_test.bin (needs a real ml_model.c — see ML section below)
```

2. **Plug in the Nucleo**

3. **Flash**
```powershell
   .\flash_and_monitor.ps1                              # flashes gas_monitor.bin
   .\flash_and_monitor.ps1 -BinName gas_monitor_test.bin # flashes the ML test build
```
   This finds the Nucleo drive, copies the chosen `.bin` to it (which
   flashes the board), finds the ST-Link virtual COM port automatically,
   and opens a live serial monitor at 115200 baud — no need to dig
   through Device Manager or PuTTY manually. Each run also saves a
   timestamped `gas_log_*.csv` in the project folder.

4. **Confirm it's running**
   It should be outputting sensor readings in the terminal window
   (plus a "Class" column if running the test firmware).

## Training the ML model

1. Collect training data: run step 3 above (`gas_monitor.bin`) once per
   condition (regular / bakingsoda / activatedcharcoal), then manually
   add a `Type` column to each session's CSV with that label.
2. Build the training image and run it on your labeled CSVs:
```powershell
   docker build -t gas-ml .
   docker run --rm -v "${PWD}:/data" -w /data gas-ml python ml/train_model.py training_csvs/*.csv
```
3. This overwrites `ml_model.c` / `ml_model.h` with the real trained model.
4. Rebuild and flash the test firmware (step 1/3 above, `make test`) to
   run live classification on the STM32.