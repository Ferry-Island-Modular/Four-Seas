# FourSeas Wavetable Oscillator

A 4-oscillator wavetable synthesizer module for Eurorack modular synthesizers, built on the Electrosmith Daisy Seed platform. Features four related outputs, SD card-based wavetable loading, and comprehensive CV control.

## Features

- **4 Related Wavetable Oscillators** with real-time morphing
- **SD Card Wavetable Loading** - Up to 12 banks × 8 pages of wavetables
- **Comprehensive CV Control** - V/Oct tracking, FM, sync, and wavetable morphing
- **RGB LED Feedback** - Visual indication of parameters and states
- **Persistent Settings** - Calibration data and user preferences stored in flash

## Hardware

### Main Controller

- **STM32H750IBK6** - ARM Cortex-M7 microcontroller (Daisy Seed)

### External ICs

- **MCP3564R** - 24-bit Delta-Sigma ADC for CV inputs
- **MCP23008** - 8-bit I2C GPIO expander for button inputs
- **TLC59116** - 16-channel LED driver for RGB LED control

### Memory

- **External SDRAM** - Wavetable storage (via Daisy Seed)
- **QSPI Flash** - Persistent settings and calibration data

## Getting Started

### Prerequisites

#### Required Tools

- **ARM GCC Toolchain** for embedded ARM development
  - Ubuntu/Debian: `sudo apt-get install gcc-arm-none-eabi`
  - macOS: `brew install --cask gcc-arm-embedded`
  - Windows: Download from [ARM's website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
- **Make** build system
  - Ubuntu/Debian: `sudo apt-get install build-essential`
  - macOS: Included with Xcode Command Line Tools
  - Windows: Use MinGW or WSL
- **Git** with submodule support

#### Cloning the Repository

This project uses git submodules for dependencies. Clone with:

```bash
# Clone with submodules
git clone --recursive https://github.com/Ferry-Island-Modular/FourSeas.git
cd FourSeas

# OR if already cloned without --recursive:
git submodule update --init --recursive
```

**Important:** This project uses a forked version of libDaisy with FourSeas-specific modifications. The submodules are configured to use the correct repositories automatically.

### Building

#### Build Commands

```bash
# Standard build
make

# Build with custom wavetable sample count
make WAVE_SAMPLES=1024
```

#### Build Configuration

- `BOARD_REV` - Hardware board revision (3 or 4, default: 4. If you need rev3, you'll already know)
- `WAVE_SAMPLES` - Samples per wavetable (default: 2048)

#### Programming/Flashing

Flash the firmware to your Daisy Seed:

```bash
make program  # using ST-Link, or...

make program-jlink  # if you have a J-Link
```

Or use the Daisy Web Programmer for DFU flashing (no debugger required):
[https://electro-smith.github.io/Programmer/](https://electro-smith.github.io/Programmer/)

## SD Card Setup

Organize wavetable files on the SD card as follows:

```
/
├── 1/
│   ├── 1.wav
│   ├── 2.wav
│   └── ... (up to 8.wav)
├── 2/
│   ├── 1.wav
│   └── ...
└── ... (up to bank 12)
```

### Wavetable Format

- **WAV files** containing wavetable data
- Each file: 8 waves × 8 columns of samples
- Default: 2048 samples per wave (configurable at compile time)
- Supports up to 12 banks with 8 pages each

## Hardware Revisions

### REV_3

- 5-position rotary switch
- Inverted output on channel A2

### REV_4

- 8-position rotary switch
- Standard output polarity

## Calibration

The FourSeas module includes a comprehensive calibration routine to ensure accurate V/Oct tracking, proper knob scaling, and correct rotary switch thresholds. Calibration data is automatically saved to flash memory.

### Starting Calibration

To enter calibration mode:

1. **Hold both OSC MODE buttons** (SW_OSC_MODE_1 and SW_OSC_MODE_2) simultaneously
2. **Keep holding for 5+ seconds** until the LED light show begins
3. All LEDs will flash 3 times to indicate calibration has started

### Calibration Steps

#### Step 1: Normalize Controls (Red LEDs)

- **LED Indicator**: All LEDs turn red
- **Action Required**:
  - Unplug all CV cables
  - Turn all knobs fully counterclockwise (minimum position)
  - Ensure no CV signals are applied
- **Purpose**: Establishes zero-point offsets for all analog controls
- **Next Step**: Press **OSC SYNC TYPE 2** button when ready

#### Step 2: Set Control Scaling (Green LEDs)

- **LED Indicator**: All LEDs turn green
- **Action Required**:
  - Turn knobs fully clockwise (maximum position)
  - Apply maximum CV signals to all CV inputs
- **Purpose**: Calibrates scaling for coarse tuning and frequency spread controls
- **Next Step**: Press **OSC SYNC TYPE 2** button when ready

#### Step 3: V/Oct Calibration - C1 Reference (Blue LEDs)

- **LED Indicator**: All LEDs turn blue
- **Action Required**:
  - Connect a **1.0V DC** precision voltage source to the **V/OCT input**
  - Ensure accurate voltage (use a calibrated multimeter if needed)
- **Purpose**: Establishes the low reference point for V/Oct tracking
- **Next Step**: Press **OSC SYNC TYPE 2** button when ready

#### Step 4: V/Oct Calibration - C3 Reference (White LEDs)

- **LED Indicator**: All LEDs turn white (fully on)
- **Action Required**:
  - Change the voltage source to **3.0V DC** (still connected to V/OCT input)
  - Ensure accurate voltage measurement
- **Purpose**: Establishes the high reference point for V/Oct tracking (2 octaves above C1)
- **Next Step**: Press **OSC SYNC TYPE 2** button when ready

#### Step 5: Rotary Switch Calibration (LEDs off initially)

- **LED Indicator**: LEDs turn on individually as each position is calibrated
- **Action Required**:
  - Rotate the rotary switch through each position
  - Press **OSC SYNC TYPE 2** button at each position
  - LEDs will light up sequentially to indicate progress
- **Purpose**: Calibrates threshold detection for rotary switch positions
- **Completion**: Process continues until all switch positions are calibrated

### Calibration Complete

- All LEDs turn off
- Calibration data is automatically saved to flash memory
- Module returns to normal operation
- V/Oct tracking, knob scaling, and switch detection are now calibrated

### Tips for Accurate Calibration

1. **Use a precision voltage source** for V/Oct calibration steps
2. **Allow the module to warm up** for 5-10 minutes before calibrating
3. **Work in a stable temperature environment**
4. **Use a quality multimeter** to verify your reference voltages
5. **Take your time** - accuracy in calibration affects long-term performance

### Troubleshooting

- **Calibration won't start**: Ensure both OSC MODE buttons are held for full 5+ seconds
- **Inaccurate V/Oct tracking**: Re-run calibration with verified precision voltage sources
- **Switch positions not detecting**: Ensure rotary switch is moved to distinct positions during step 5

## Development

### Debugging

- **J-Link** debugger support
- SWO output available for printf debugging
- GPIO toggle points for oscilloscope timing analysis

### Performance Profiling with J-Link and Orbuculum

The FourSeas project supports real-time performance profiling using J-Link's SWO (Serial Wire Output) and the Orbuculum toolchain. This workflow enables detailed analysis of CPU usage, function call patterns, and performance bottlenecks.

#### Prerequisites

1. **J-Link** debugger with SWO support
2. **Orbuculum** toolchain - Build from source: [https://github.com/orbcode/orbuculum](https://github.com/orbcode/orbuculum)
3. **ITM-Tools** for data processing - Build from source: [https://github.com/orbcode/itm-tools](https://github.com/orbcode/itm-tools)
4. **ARM GCC toolchain** with GDB

#### Profiling Workflow

##### Step 1: Start J-Link GDB Server

```bash
/usr/local/bin/JLinkGDBServerCL -singlerun -nogui -if jtag -port 50000 -swoport 50001 -telnetport 50002 -device STM32H750IB
```

**Parameters:**

- `-singlerun`: Terminate after one GDB session
- `-nogui`: Run without graphical interface
- `-if jtag`: Use JTAG interface
- `-port 50000`: GDB server port
- `-swoport 50001`: SWO data port
- `-telnetport 50002`: Telnet port
- `-device STM32H750IB`: Target MCU

##### Step 2: Connect GDB and Enable SWO

```bash
arm-none-eabi-gdb build/FourSeas_rev4_2048.elf -x fs_ocd.cfg
```

The `fs_ocd.cfg` configuration file:

- Connects to J-Link GDB server on port 2331
- Configures SWO for 480MHz system clock with 100kHz output
- Enables DWT (Data Watchpoint and Trace) for PC sampling
- Sets up ITM (Instrumentation Trace Macrocell) for printf output

##### Step 3: Real-time Profiling with Orbtop

In a separate terminal, run the real-time profiler:

```bash
../../orbuculum/build/orbtop -e build/FourSeas_rev4_2048.elf -s localhost:2332 -c 25 -E -v3
```

**Parameters:**

- `-e`: ELF file with debug symbols
- `-s localhost:2332`: SWO data source (orbuculum server)
- `-c 25`: Update interval in Hz
- `-E`: Enable PC sampling
- `-v3`: Verbose output level

##### Step 4: Capture SWO Data to File

For offline analysis, stream SWO data to a file:

```bash
nc localhost 2332 > swo_file_Ofast
```

##### Step 5: Process Captured Data

Convert raw SWO data to human-readable format:

```bash
../../itm-tools/target/debug/pcsampl -e build/FourSeas_rev4_2048.elf swo_file_Ofast 2>/dev/null > pcsampl_Ofast.log
```

#### Analyzing Results

The processed `pcsampl_*.log` files contain:

- **Function call frequencies** - Which functions consume the most CPU time
- **Call stack analysis** - Function call patterns and relationships
- **Performance hotspots** - Areas of code that may need optimization
- **Timing statistics** - Execution time distributions

#### Profiling Tips

1. **Build with optimization**: Use `-O2` or `-O3` for realistic performance measurements
2. **Warm-up period**: Allow the system to stabilize before capturing data
3. **Sufficient sample duration**: Capture 30+ seconds of data for meaningful statistics
4. **Compare configurations**: Profile different build configurations to measure optimization impact
5. **Focus on audio callback**: Pay special attention to `AudioCallback` function performance

#### Troubleshooting

- **No SWO data**: Verify J-Link connection and SWO pin configuration
- **Garbled output**: Check SWO clock speed matches system frequency
- **Missing symbols**: Ensure ELF file includes debug information (`-g` flag)
- **High CPU usage**: SWO data collection itself can impact performance

#### File Organization

Generated profiling files follow this naming convention:

- `swo_file_*`: Raw SWO data captures
- `pcsampl_*.log`: Processed profiling results
- `fs_ocd.cfg`: GDB configuration for SWO setup

### Code Structure

- `FourSeasHW` - Hardware abstraction layer
- `Ui` - User interface and parameter management
- `WavetableOscillator` - Synthesis engine
- `Settings/AppState` - Persistent storage management

### Dependencies

This project includes the following libraries as git submodules:

- **libDaisy** - Daisy Seed hardware abstraction
  - Uses Ferry-Island-Modular fork with FourSeas-specific modifications
  - Modifications include: I2C4 peripheral fixes, SDRAM timing, audio SAI handling, crash logging
- **DaisySP** - Digital signal processing library (upstream)
- **stmlib** - Parameter interpolation and utilities (upstream)
- **FatFS** - SD card file system support (included via libDaisy)

## License

MIT License - See [LICENSE](LICENSE) file for details.

FourSeas is built on top of:
- libDaisy (MIT License) - [Ferry-Island-Modular fork](https://github.com/Ferry-Island-Modular/libDaisy)
- DaisySP (MIT License) - [Electrosmith](https://github.com/electro-smith/DaisySP)
- stmlib (MIT License) - [Mutable Instruments](https://github.com/pichenettes/stmlib)

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

### Development Guidelines

- Follow the existing code style (clang-format configuration included)
- Test on hardware before submitting PRs
- Document any new features or changes to existing behavior
- Keep commits focused and atomic
