# Yeast Fermentation Treatment Detection
Stream gas sensor readings from live yeast fermentation offgases while detecting air environment with machine learning classification model based on composition of air environment including carbon dioxide and ethanol alcohol gas. 

The prior project was: Sensor-Monitoring-for-Chemical-Reaction-with-Yeast (Sense gases outputted from yeast reaction with sugar water, combining a chemistry lab experiment with prototype electronics.)

## Lab
- (Assemble the electronics)
- First do a control run on the sensors with regular air
- Put yeast in a container
- Mix sugar into water
- Pour the mixture onto the yeast
- Put a lid only half-covering the yeast container (do not seal the lid)
- Hang the sensors inside the container (do not let them dip into the wet yeast in the container)
- Now run the program with the sensors hanging inside at the top of the container

## Technology
The respective target gas of the sensor causes the sensor's voltage resistance to go down, causing the sensor to send higher voltage to the stm32.
- STM32 F446 Microcontroller $30
- MQ-3 Alcohol Gas Sensor $10
- MQ-135 Air Quality Hazardous Gas Sensor $20
- Breadboard $3
- Jumpers MtF and FtF $3
- Resistors 10kOhm and 22kOhm/20kOhm $3

## Code

```
08_reaction_gas_sensing/
├── ml/
    ├── Dockerfile
    ├── train_model.py
    ├── train.ps1
    └── train_data.csv
├── detect.c
├── detect.h
├── flash_and_monitor.ps1
├── link.ld
├── main.c
├── main_test.c
├── ml_model.c
├── ml_model.h
├── Makefile
├── startup.c
├── stm32f446.h
├── system.c
└── system.h
```

## Commands
```powershell
make clean
make
.\flash_and_monitor.ps1
```

If the flash and monitor script doesn't work and the terminal freezes after running the script then unplug your STM32 microcontroller and replug it back in.

## Full Steps

```powershell
cd folder/projectfolder
```

1. **Build**
   ```powershell
   make
   ```
   Produces `gas_monitor.bin` and `gas_monitor.elf` in folder.

2. **Plug in the Nucleo**

3. **Flash**
   ```powershell
   .\flash_and_monitor.ps1
   ```
   This finds the Nucleo drive, copies `gas_monitor.bin` to it (which
   flashes the board), finds the ST-Link virtual COM port automatically,
   and opens a live serial monitor at 115200 baud — no need to dig
   through Device Manager or PuTTY manually.

4. **Confirm it's running**
   It should be outputting sensor readings in the terminal window.

## Training the ML Model

Label your collected data with a `Type` column (`openair`, `regular`,
`activatedcharcoal`, etc.) and save it as `ml/train_data.csv`. Then,
with Docker Engine running:

```powershell
.\ml\train.ps1
```

This builds the training image, runs RandomForest training, and
overwrites `ml_model.c` / `ml_model.h` with the compiled model — no
runtime needed on the STM32, it's plain C.

## Running the Live Test

```powershell
make clean
make test
.\flash_and_monitor.ps1 -BinName gas_monitor_test.bin
```

Flashes the ML-enabled firmware. The terminal table now includes a
**Class** column showing the live predicted condition per reading.