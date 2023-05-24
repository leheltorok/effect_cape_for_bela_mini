Delay_Chain is a time-based audio effects chain, designed to run on Bela Mini in combination with the effect_cape.

This project serves as a demonstration of utilizing the effect_cape in a multi-effect configuration and provides all the necessary frameworks to operate the hardware.

The main.cpp file in the OLED directory is part of the O2O example (available at: https://github.com/giuliomoro/O2O) and has been modified to fit the Delay_Chain project.

To run the display you need to download the full repository from the link above and upload it as a new project to your board. For instructions refer to: https://learn.bela.io/using-bela/bela-techniques/using-an-oled-screen/

(Don’t forget to replace the existing main.cpp file with the one located in the OLED folder.)

To operate the screen alongside the Delay_Chain project, you will have to set it up to run as a service at boot, by following the instructions provided in this guide: https://learn.bela.io/using-bela/bela-techniques/running-a-program-as-a-service/
