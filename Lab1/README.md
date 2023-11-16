# Hourglass

This project is a hourglass timer in a LED strip, with configurable time and personalized colors and transitions.

## Hardware

The hardware used in this project is a Raspberry Pi Pico board and a WS2812B LED strip.

## Software

The software used in this project is the Arduino Framework using PlatformIO extension on VSCode.

## Usage

### Hardware

The hardware setup is very simple. The LED strip is connected to the Raspberry Pi Pico board as follows:

![Schematic of connections](/Lab1/Schematic_ACE_lab1_2023-11-08.png "Schematic of connections")

### Configurations

The number of LEDs in the LED strip can be configured in the define `NUM_LEDS` in the beginning of the code. The default value is 5.

```c++
#define NUM_LEDS 5
```

### Controls

The Raspberry Pi Pico board's buttons allow the user to operate the hourglass timer. To reset the timer if it is currently running, use the Sgo button. To increase one LED in the hourglass timer, press the Sup button. The Sdown button is used to pause and unpause the timer that is currently in use.

The user enters setup mode if they hold down the Sup button for three seconds. By pushing the Sdown button, the user may adjust the duration of each LED in this mode, choosing from 1, 2, 5, or 10 seconds. By pressing the Sup button, the user is able to customize the transition type, choosing between the default, blinking, and fade out options while still utilizing the Sdown button. The color of the LEDs may be changed by pressing the Sup button. The user has three seconds to hold down the Sup button in order to get out of configuring mode.

The LED strip goes into IDLE mode every thirty seconds if the user doesn't hit any buttons. The user can go back to their former state by pressing any button.

### State machines

The program is divided in four state machines:

- **MAIN**: the main state machine, which controls the other state machines and the LED strip;
![MAIN state machine](/Lab1/mainn.png "MAIN State machine")
- **COUNTDOWN**: the state machine that displays the hourglass timer;
![COUNTDOWN state machine](/Lab1/countdownn.png "COUNTDOWN State machine")
- **AUX**: the state machine that is used to auxiliate the other state machines;
![AUX state machine](/Lab1/auxx.png "AUX State machine")
- **CONFIG**: the state machine that is used to configure the hourglass timer.
![CONFIG state machine](/Lab1/configg.png "CONFIG State machine")


