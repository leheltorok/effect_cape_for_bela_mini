
##Guide for assembly

The PCB of the effect_cape is not available for commercial sale, so you will have to order it yourself. In order to do so, you will only have to submit the file ‘effect_cape_rev1.kicad_pcb’ to any company that specializes in manufacturing PCB prototypes. (this will be the cheapest and fastest way to get your PCB)

After you have the PCB and all parts (see BOM) you can start the assembly.

For the exact position of the parts refer to the interactive BOM: https://www.dropbox.com/s/17bvmpcd8jzs7nz/ibom.html?dl=0

Step-by-step:

Step 1. Add resistors R1, R2, R3, R4, R5, R6, R8, R9, R10, R11, R12, R13, R14, R15 (10k) for the rotary encoders and switches and R7 (240R) for the LED.

Step 2. Attach both 2x18 male headers (J1, J2) to the board, followed by the 1x1 male header (J11) connection to A0 and the two 1x3 sockets (J3, J4) connecting the audio In and Outputs.

Step 3. Attach 3x NEUTRIK NR-J4HF to the audio input and outputs, 1x NEUTRIK NR-J6HF to the expression pedal input and 1x CUI SJ1-3535NG to the headphones output.

Step 4. Add the DC Jack Barrel (J12) and the Push Button (SW). (optional - include only if you need this type of power supply)

Step 5. Add the 2x2 male header (J7) onto the PCB. Securely fasten both (PF4520) Foot Switches in their designated positions. Connect the pins to the legs of the switches.

Step 6. Attach the Rotary Encoders (Enc_1, Enc_2, Enc_3, Enc_4). Make sure that their bottom side doesn’t touch any of the pins beneath them.

Step 7. Add the LED and cut its legs. The red anode should be attached to digital pin 8 and the blue anode to digital pin 10. (this can be also swapped in the code)

Step 8. Attach the OLED and you are done.

Now you can attach your Bela Mini to the cape and give it a try.
