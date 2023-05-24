/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/
/**
\example Communication/oled-screen/main.cpp

Working with OLED Screens and OSC
---------------------------------

This example shows how to interface with an OLED screen.

We use the u8g2 library for drawing on the screen.

This program is an OSC receiver which listens for OSC messages which can be sent
from another programmes on your host computer or from your Bela project running
on the board.

This example comes with a Pure Data patch which can be run on your host computer
for testing the screen. Download local.pd from the project files. This patch
sends a few different OSC messages which are interpreted by the cpp code which
then draws on the screen. We will now explain what each of the osc messages does.

Select the correct constructor for your screen (info to be updated). When you run
this project the first thing you will see is a Bela logo!

The important part of this project happens in the parseMessage() function which
we'll talk through now.

/osc-test

This is just to test whether the OSC link with Bela is functioning. When this message
is received you will see "OSC TEST SUCCESS!" printed on the screen. In the PD patch
click the bang on the left hand side. If you don't see anything change on the screen
then try clicking the `connect bela.local 7562` again to reestablish the connection.
Note that you will have to do this every time the Bela programme restarts.

/number

This prints a number to the screen which is received as a float. Every time you click the bang
above this message a random number will be generated and displayed on the screen.

/display-text

This draws text on the screen using UTF8.

/parameters

This simulates the visualisation of 3 parameters which are generated as ramps in
the PD patch. They are sent over as floats and then used to change the width of
three filled boxes. We also write to the screen for the parameter labelling directly
from the cpp file.

/lfos

This is similar to the /parameters but with a different animation and a different
means of taking a snapshot of an audio signal (see the PD Patch). In this case the
value from the oscillator is used to scale the size on an ellipse.

/waveform

This is most flexible message receiver. It will draw any number of floats (up to
maximum number of pixels of your screen) that are sent after /waveform. In the PD
patch we have two examples of this. The first sends 5 floats which are displayed
as 5 bars. No that the range of the screen is scaled to 0.0 to 1.0.

The second example stores the values of an oscillator into an array of 128 values
(the number of pixels on our OLED screen, update the array size in the PD patch if you
have a different sized screen). These 128 floats (between 0.0 and 1.0) are sent ten
times a second.
*/

#include <signal.h>
#include <libraries/OscReceiver/OscReceiver.h>
#include <unistd.h>
#include "u8g2/U8g2LinuxI2C.h"
#include <vector>
#include <algorithm>

const unsigned int gI2cBus = 1;

// #define I2C_MUX // allow I2C multiplexing to select different target displays
struct Display {U8G2 d; int mux;};
std::vector<Display> gDisplays = {
	// use `-1` as the last value to indicate that the display is not behind a mux, or a number between 0 and 7 for its muxed channel number
	{U8G2LinuxI2C(U8G2_R0, gI2cBus, 0x3c, u8g2_Setup_ssd1306_i2c_128x64_noname_f), -1},
	// add more displays / addresses here
};

unsigned int gActiveTarget = 0;
const int gLocalPort = 7562; //port for incoming OSC messages

#ifdef I2C_MUX
#include "TCA9548A.h"
const unsigned int gMuxAddress = 0x70;
TCA9548A gTca;
#endif // I2C_MUX

/// Determines how to select which display a message is targeted to:
typedef enum {
	kTargetSingle, ///< Single target (one display).
	kTargetEach, ///< The first argument to each message is an index corresponding to the target display
	kTargetStateful, ///< Send a message to /target <float> to select which is the active display that all subsequent messages will be sent to
} TargetMode;

TargetMode gTargetMode = kTargetSingle; // can be changed with /targetMode
OscReceiver oscReceiver;
int gStop = 0;

// Handle Ctrl-C by requesting that the audio rendering stop
void interrupt_handler(int var)
{
	gStop = true;
}

static void switchTarget(int target)
{
#ifdef I2C_MUX
	if(target >= gDisplays.size())
		return;
	U8G2& u8g2 = gDisplays[target].d;
	int mux = gDisplays[target].mux;
	static int oldMux = -1;
	if(oldMux != mux)
		gTca.select(mux);
	oldTarget = target;
#endif // I2C_MUX
	gActiveTarget = target;
}

int parseMessage(oscpkt::Message msg, const char* address, void*)
{

	oscpkt::Message::ArgReader args = msg.arg();
	enum {
		kOk = 0,
		kUnmatchedPattern,
		kWrongArguments,
		kInvalidMode,
		kOutOfRange,
	} error = kOk;
	printf("Message from %s\n", address);
	bool stateMessage = false;
	// check state (non-display) messages first
	if (msg.match("/target")) {
		stateMessage = true;
		if(kTargetStateful != gTargetMode) {
			fprintf(stderr, "Target mode is not stateful, so /target messages are ignored\n");
			error = kInvalidMode;
		} else {
			int target;
			if(args.popNumber(target).isOkNoMoreArgs()) {
				printf("Selecting /target %d\n", target);
				switchTarget(target);
			} else {
				fprintf(stderr, "Argument to /target should be numeric (int or float)\n");
				error = kWrongArguments;
			}
		}
	} else if (msg.match("/targetMode")) {
		stateMessage = true;
		int mode;
		if(args.popNumber(mode).isOkNoMoreArgs())
		{
			if(mode != kTargetSingle && mode != kTargetStateful && mode != kTargetEach)
				error = kOutOfRange;
			else {
				gTargetMode = (TargetMode)mode;
				printf("Target mode: %d\n", mode);
			}
		} else
			error = kWrongArguments;
	}
	if(gActiveTarget >= gDisplays.size())
	{
		fprintf(stderr, "Target %u out of range. Only %u displays are available\n", gActiveTarget, gDisplays.size());
		return 1;
	}
	U8G2& u8g2 = gDisplays[gActiveTarget].d;
	u8g2.clearBuffer();
	int displayWidth = u8g2.getDisplayWidth();
	int displayHeight = u8g2.getDisplayHeight();
	if(!stateMessage && kTargetEach == gTargetMode)
	{
		// if we are in kTargetEach and the message is for a display, we need to peel off the
		// first argument (which denotes the target display) before processing the message
		int target;
		if(args.popNumber(target))
		{
			switchTarget(target);
		} else {
			fprintf(stderr, "Target mode is \"Each\", therefore the first argument should be an int or float specifying the target display\n");
			error = kWrongArguments;
		}
	}

	// code below MUST use msg.match() to check patterns and args.pop... or args.is ... to check message content.
	// this way, anything popped above (if we are in kTargetEach mode), won't be re-used below
	if(error || stateMessage) {
		// nothing to do here, just avoid matching any of the others
	} else if (msg.match("/scanner_vibrato"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /scanner_vibrato %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.1015, displayHeight * 0, "[SCANNER_VIBRATO]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx1]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[rate]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[dpth]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/tape_delay_1"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /tape_delay_1 %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[fbck]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/tape_delay_2"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /tape_delay_2 %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[ramp]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[roll]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/freeverb"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /freeverb %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.265, displayHeight * 0, "[FREEVERB]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx3]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[damp]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/looper"))
	{
		int number1;

		if(args.popNumber(number1).isOkNoMoreArgs())

		{

			printf("received /looper %d\n", number1);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.31, displayHeight * 0, "[LOOPER]");
			
			
		    u8g2.drawStr(displayWidth * 0.33, displayHeight * 0.40, "[     ]");
			u8g2.drawUTF8(displayWidth * 0.41, displayHeight * 0.40, std::to_string(number1).c_str());
			u8g2.drawStr(displayWidth * 0.33, displayHeight * 0.60, "[level]");
			
			


	
		}



		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//EXPRESSION//SCANNER//
		
	} else if (msg.match("/d/w_scn_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /d/w_scn_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.1015, displayHeight * 0, "[SCANNER_VIBRATO]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx1]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[rate]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[dpth]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/scanner_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /scanner_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.1015, displayHeight * 0, "[SCANNER_VIBRATO]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx1]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[rate]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[dpth]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/rate_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /rate_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.1015, displayHeight * 0, "[SCANNER_VIBRATO]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx1]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[rate]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.15, displayHeight * 0.85, "exp");
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[dpth]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/depth_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /depth_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.1015, displayHeight * 0, "[SCANNER_VIBRATO]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx1]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[rate]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[dpth]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.85, "exp");
	
	
		}
		
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//EXPRESSION//DELAY_1//
		
	} else if (msg.match("/d/w_del_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /d/w_del_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[fbck]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/delay_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /delay_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[fbck]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
		}
		
	} else if (msg.match("/deltime_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /deltime_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.15, displayHeight * 0.85, "exp");
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[fbck]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/feedback_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /feedback_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[fbck]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.85, "exp");
	
	
		}
		
					///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//EXPRESSION//DELAY_2//
		
	} else if (msg.match("/d/w_del_2_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /d/w_del_2_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[ramp]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[roll]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/delay_2_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /delay_2_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[ramp]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[roll]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
		}
		
		
	} else if (msg.match("/ramptime_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /ramptime_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[ramp]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.15, displayHeight * 0.85, "exp");
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[roll]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/rolloff_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /rolloff_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.225, displayHeight * 0, "[TAPE_DELAY]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx2]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[ramp]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[roll]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.85, "exp");
	
		}
		
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//EXPRESSION//REVERB//
		
	} else if (msg.match("/d/w_rev_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /d/w_rev_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.265, displayHeight * 0, "[FREEVERB]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx3]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[damp]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/reverb_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /reverb_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.265, displayHeight * 0, "[FREEVERB]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx3]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[exp]");
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[damp]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/revtime_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /revtime_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.265, displayHeight * 0, "[FREEVERB]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx3]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[time]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.15, displayHeight * 0.85, "exp");
		
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[damp]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.66, displayHeight * 0.85, std::to_string(number4).c_str());
	
	
		}
		
	} else if (msg.match("/damping_exp"))
	{
		int number1;
		int number2;
		int number3;
		int number4;
		if(args.popNumber(number1).popNumber(number2).popNumber(number3).popNumber(number4).isOkNoMoreArgs())

		{

			printf("received /damping_exp %d %d %d %d\n", number1 ,number2, number3, number4);
			u8g2.setFont(u8g2_font_6x12_tf);
			u8g2.drawStr(displayWidth * 0.265, displayHeight * 0, "[FREEVERB]");
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.25, "[d/w]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.45, std::to_string(number1).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.25, "[fx1]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.45, "[   ]");
			u8g2.drawUTF8(displayWidth * 0.71, displayHeight * 0.45, std::to_string(number2).c_str());
		
			
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.65, "[rate]");
			u8g2.drawStr(displayWidth * 0.1, displayHeight * 0.85, "[    ]");
			u8g2.drawUTF8(displayWidth * 0.15, displayHeight * 0.85, std::to_string(number3).c_str());
			
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.65, "[dpth]");
			u8g2.drawStr(displayWidth * 0.61, displayHeight * 0.85, "[    ]");
			u8g2.drawStr(displayWidth * 0.66, displayHeight * 0.85, "exp");
	
	
		}
		

		
	} else if (msg.match("/desel_oled"))
	{
		if(!args.isOkNoMoreArgs()){
			error = kWrongArguments;
		}
		else {
			printf("received /desel_oled\n");
		u8g2.setFont(u8g2_font_4x6_tf);
		u8g2.setFontRefHeightText();
		u8g2.setFontPosTop();
		u8g2.drawStr(0, 0, " ____  _____ _        _");
		u8g2.drawStr(0, 7, "| __ )| ____| |      / \\");
		u8g2.drawStr(0, 14, "|  _ \\|  _| | |     / _ \\");
		u8g2.drawStr(0, 21, "| |_) | |___| |___ / ___ \\");
		u8g2.drawStr(0, 28, "|____/|_____|_____/_/   \\_\\");
		}
		



	} else
		error = kUnmatchedPattern;
	if(error)
	{
		std::string str;
		switch(error){
			case kUnmatchedPattern:
				str = "no matching pattern available\n";
				break;
			case kWrongArguments:
				str = "unexpected types and/or length\n";
				break;
			case kInvalidMode:
				str = "invalid target mode\n";
				break;
			case kOutOfRange:
				str = "argument(s) value(s) out of range\n";
				break;
			case kOk:
				str = "";
				break;
		}
		fprintf(stderr, "An error occurred with message to: %s: %s\n", msg.addressPattern().c_str(), str.c_str());
		return 1;
	} else
	{
		if(!stateMessage)
			u8g2.sendBuffer();
	}
	return 0;
}

int main(int main_argc, char *main_argv[])
{
	if(0 == gDisplays.size())
	{
		fprintf(stderr, "No displays in gDisplays\n");
		return 1;
	}
#ifdef I2C_MUX
	if(gTca.initI2C_RW(gI2cBus, gMuxAddress, -1) || gTca.select(-1))
	{
		fprintf(stderr, "Unable to initialise the TCA9548A multiplexer. Are the address and bus correct?\n");
		return 1;
	}
#endif // I2C_MUX
	for(unsigned int n = 0; n < gDisplays.size(); ++n)
	{
		switchTarget(n);
		U8G2& u8g2 = gDisplays[gActiveTarget].d;
#ifndef I2C_MUX
		int mux = gDisplays[gActiveTarget].mux;
		if(-1 != mux)
		{
			fprintf(stderr, "Display %u requires mux %d but I2C_MUX is disabled\n", n, mux);
			return 1;
		}
#endif // I2C_MUX
		u8g2.initDisplay();
		u8g2.setPowerSave(0);
		u8g2.clearBuffer();
		u8g2.setFont(u8g2_font_4x6_tf);
		u8g2.setFontRefHeightText();
		u8g2.setFontPosTop();
		u8g2.drawStr(0, 0, " ____  _____ _        _");
		u8g2.drawStr(0, 7, "| __ )| ____| |      / \\");
		u8g2.drawStr(0, 14, "|  _ \\|  _| | |     / _ \\");
		u8g2.drawStr(0, 21, "| |_) | |___| |___ / ___ \\");
		u8g2.drawStr(0, 28, "|____/|_____|_____/_/   \\_\\");
		if(gDisplays.size() > 1)
		{
			std::string targetString = "Target ID: " + std::to_string(n);
			u8g2.drawStr(0, 50, targetString.c_str());
		}
		u8g2.sendBuffer();
	}
	// Set up interrupt handler to catch Control-C and SIGTERM
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);
	// OSC
	oscReceiver.setup(gLocalPort, parseMessage);
	while(!gStop)
	{
		usleep(100000);
	}
	return 0;
}
