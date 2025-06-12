# Knob Frames Project

This project contains code for controlling LED strips using potentiometer knobs.

## Contents

- `src/` - Contains the main Arduino code for the project
- `test/` - Contains test and simulation tools
  - `led_simulator/` - Python-based simulator for visualizing the LED patterns

## LED Strip Simulator

A Python-based simulator is available in the `test/led_simulator/` directory. This simulator allows you to visualize how the LED strip would look with different parameter settings.

To run the simulator:

```bash
python test/led_simulator/matplotlib_led_simulator.py
```

See the [LED Simulator README](test/led_simulator/README.md) for more details.
