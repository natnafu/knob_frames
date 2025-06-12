# LED Strip Simulator

This project provides Python applications that simulate an LED strip display using sine waves to create colorful patterns, similar to the Arduino implementation in the original project. The simulators provide graphical user interfaces with sliders to control the parameters of the red, green, and blue sine waves.

## Features

- Real-time visualization of LED strip patterns
- Interactive sliders to control:
  - Speed: How fast the wave moves (0.0 to 0.5 LEDs/second)
  - Wavelength: The length of one complete wave cycle (0 to 400 LEDs)
  - Brightness: The intensity of each color (0 to 255)
- Visualization of the sine wave patterns for each color channel
- Multiple simulator versions to choose from

## Requirements

- Python 3.6 or higher
- Required Python packages (listed in requirements.txt):
  - numpy
  - matplotlib
  - Pillow (PIL)

## Installation

1. Clone or download this repository
2. Install the required packages:

```bash
pip install -r test/led_simulator/requirements.txt
```

## Usage

### Matplotlib-based Simulator (Recommended)

The matplotlib-based simulator provides the most reliable visualization and controls:

```bash
python test/led_simulator/matplotlib_led_simulator.py
```

This version uses matplotlib to create a visualization of the LED strip with sliders for controlling all parameters.

### Tkinter-based Simulator

A Tkinter-based version is also available, but may not work on all systems:

```bash
python test/led_simulator/led_strip_simulator.py
```

### Simple Simulator

A simplified version with basic visualization:

```bash
python test/led_simulator/simple_led_simulator.py
```

## Controls

- **Speed sliders**: Control how fast the wave pattern moves along the LED strip
- **Wavelength sliders**: Control the length of one complete wave cycle
- **Brightness sliders**: Control the intensity of each color

## Tips

- Set wavelength to 0 to create a pulsing effect that doesn't move along the strip
- Try different combinations of speeds and wavelengths to create interesting patterns
- The simulator shows one strip of 144 LEDs for clarity, while the original project uses 5 strips

## Relationship to Original Arduino Code

These simulators replicate the functionality of the Arduino code that controls physical LED strips. They implement the same algorithms for:

- Sine wave generation for each color channel
- Color mixing to create the final LED colors
- Parameterized control of speed, wavelength, and brightness

The main difference is that these simulators use sliders instead of physical potentiometer knobs to control the parameters.
