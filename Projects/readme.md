**Delay_Chain is a time-based audio effects chain, designed to run on Bela Mini in combination with the effect_cape.**

This project serves as a demonstration of utilizing the effect_cape in a multi-effect configuration and provides all the necessary frameworks to operate the hardware.



**The audio effects included are as follows:** (their order is interchangeable in the _main patch)

[scanner] Hammond's Scanner Vibrato simulator with an adjustable rate, depth, and mix.

[tapedelay] Tape delay simulator with variable-speed tape and saturation.

[freeverb] Schroeder reverberator implementation using the Freeverb algorithm.

[looper] Looper with overdub capability and clock synchronized to the first layer.

(For more information on each patch, see the content within.)


**Code for executing all the hardware functionality:**

Custom libpd render.cpp to read and send 4 rotary encoder values to a Pure Data patch.

[interface] Read and address digital data received from the effect_cape.

[encoder_in] Receive and interpret initialized encoder value from the render.cpp file.

[encoder] Set up a separate control value for each parameter.

[expr_in] Read and route expression pedal signal connected to Analog In 0

[fx_sel] Skip back and forth between effects.

[led] Control bicolor LED to provide visual feedback for different states and changes.

[oled] Transmit OSC messages to drive OLED screen connected to the I2C2 pins.

[gui] Display and control parameters via a smartphone or tablet.

(For more information on each patch, see the content within.)


**Interacting with the physical interface:**

Effect_cape features four rotary encoders, each equipped with a switch, as well as two footswitches, all of which can be assigned based on your preferences.

**Skip back and forth between effects:**

In the case of the Delay_Chain project, the bottom left and right rotary encoder switches are utilized to alternate between effects in both forward and backward directions.

**Accessing alternative parameters:**

To access alternative parameters for a specific effect, press the top right rotary encoder switch. In the case of the Delay_Chain project, this functionality is applicable only to the tape delay, which includes two additional parameters in addition to the initial ones. When in alternative mode, the LED color changes to purple, indicating the activation of this feature.


**Assign the expression pedal to a specific parameter:**

The top left rotary encoder switch is designated to enable/disable the expression mode. Pressing this switch automatically selects and links the assigned parameter to the expression pedal for control. By pressing the buttons of the other encoders, you can choose the parameter you want to control. To exit the expression mode, press the top left button again. During expression mode, the LED indicator turns red, and the screen displays "[exp]" instead of showing the value of the selected parameter. When deselecting a parameter, its value will be updated to the last received value from the expression pedal.


**Accessing the GUI:**


In order to access the GUI, you need to establish an internet connection between your device and Bela. The simplest method is to connect a Wi-Fi dongle to Bela's USB port and configure it to automatically connect to your phone's hotspot. 
For instructions refer to: https://learn.bela.io/using-bela/bela-techniques/connecting-to-wifi/

Once establishing a network connection with the board, you can launch the IDE in order to access the GUI.
(By adding it to your home screen, you can transform the GUI into a mobile app.)

Up until now, I have been using the TPLINK TL-WN725N WLAN-Adapter, which cannot function as a wireless access point but can connect to other devices' hotspots.


**Using the OLED screen:**

The main.cpp file in the OLED directory is part of the O2O example (available at: https://github.com/giuliomoro/O2O) and has been modified to fit the Delay_Chain project.

To run the display you need to download the full repository from the link above and upload it as a new project to your board. For instructions refer to: https://learn.bela.io/using-bela/bela-techniques/using-an-oled-screen/

(Don’t forget to replace the existing main.cpp file with the one located in the OLED folder.)

To operate the screen alongside the Delay_Chain project, you will have to set it up to run as a service at boot, by following the instructions provided in this guide: https://learn.bela.io/using-bela/bela-techniques/running-a-program-as-a-service/


![routing_diagram](/Hardware/routing_diagram.jpg "routing_diagram")
