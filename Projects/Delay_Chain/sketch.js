// Graphical user interface (GUI) for displaying and controlling parameters via a smartphone or tablet.

// Based on the gui-send and gui-receive example codes.
// https://learn.bela.io/tutorials/pure-data/communication/controlling-bela-from-a-gui/
// https://learn.bela.io/tutorials/pure-data/communication/creating-a-gui-visualisation/

// Additional information regarding the Bela GUI at:
// https://learn.bela.io/the-ide/crafting-guis/


function setup() {
	
createCanvas(windowWidth, windowHeight);
Bela.control.send({windowWidth: windowWidth, windowHeight: windowHeight});
    
}


function draw () {
	
	

    // Draw a black background with opacity
    background(0, 50);
    
    // Retreive the data being sent from gui.pd
    let encoder = Bela.data.buffers[0];
    let fx = encoder[5]
    let exp = encoder[10]

        
    let center = windowWidth/2 -2.5;
    
      for (var n = 0; n < touches.length; n++) {

	Bela.control.send({mouseX: mouseX, mouseY: mouseY});
	
  }
    
    // Change effect if value 5 changed
    
		textSize(35);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER, TOP);
	
		if (fx === 1) {text('SCANNER VIBRATO', 0, 50, width);}
	else if (fx === 2) {text('TAPE DELAY', 0, 50, width);}
	else if (fx === 2.5) {text('TAPE DELAY', 0, 50, width);}
	else if (fx === 3) {text('FREEVERB', 0, 50, width);}
	else if (fx === 4) {text('4xLOOPER', 0, 50, width);}

	// Text and Value for encoder[1]

		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
	if (fx === 4) {text('[loop_1]', -windowWidth/4, windowHeight/2 -50, width);}
	else {text('[dry/wet]', -windowWidth/4, windowHeight/2 -50, width);}
	
		textSize(30);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
	if (exp === 1) {text('[exp]', -windowWidth/4, windowHeight/2 -135, width);}
    else {text(abs(encoder[6]), -windowWidth/4, windowHeight/2 -135, width);}
	
	// Text and Value for encoder[2]
	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
	
	 if (fx === 4) {text('[loop_2]', +windowWidth/4, windowHeight/2 -50, width);}
	else {text('[fx-level]', +windowWidth/4, windowHeight/2 -50, width);}
	
	
		textSize(30);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
	
	if (exp === 2) {text('[exp]', +windowWidth/4, windowHeight/2 -135, width);}
	else if (fx === 4) {text('', +windowWidth/4, windowHeight/2 -135, width);}
	else {text(abs(encoder[7]), +windowWidth/4, windowHeight/2 -135, width);}
	
	// Text and Value for encoder[3]
	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
	
		if (fx === 1) {text('[rate]', -windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 2) {text('[time]', -windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 2.5) {text('[ramp]', -windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 3) {text('[time]', -windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 4) {text('[loop_3]', -windowWidth/4, windowHeight/2 +195, width);}

		textSize(30);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);

	if (exp === 3) {text('[exp]', -windowWidth/4, windowHeight/2 +110, width);}
	else if (fx === 4) {text('', -windowWidth/4, windowHeight/2 +110, width);}
	else {text(abs(encoder[8]), -windowWidth/4, windowHeight/2 +110, width);}

	
	// Text and Value for encoder[4]
	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
	
		if (fx === 1) {text('[depth]', +windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 2) {text('[feedback]', +windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 2.5) {text('[rolloff]', +windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 3) {text('[damping]', +windowWidth/4, windowHeight/2 +195, width);}
	else if (fx === 4) {text('[loop_4]', +windowWidth/4, windowHeight/2 +195, width);}
	
		textSize(30);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER);
			
	if (exp === 4) {text('[exp]', +windowWidth/4, windowHeight/2 +110, width);}
	else if (fx === 4) {text('', +windowWidth/4, windowHeight/2 +110, width);}
	else {text(abs(encoder[9]), +windowWidth/4, windowHeight/2 +110, width);}

	
	// Buttons on the bottom of screen
	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER, BOTTOM);
	text('[>]', +windowWidth/3, windowHeight -25, width);
	
	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER, BOTTOM);
	text('[<]', -windowWidth/3, windowHeight -25, width);

	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER, BOTTOM);
	if (fx === 2) {text('[v]', 0, windowHeight -25, width);}
	
		textSize(25);
	strokeWeight(0.5);
	line(0, 12, width, 12);
	textAlign(CENTER, BOTTOM);
	if (fx === 2.5) {text('[^]', 0, windowHeight -25, width);}
	
        
    // Draw circles whose size changes with the value received from gui.pd   
    
    noFill();
	stroke(255);
    strokeWeight(10);
        
    for (var i = 1; i < 2; i++) {
      const thickness = 15 * abs(encoder[i]);
      strokeWeight(thickness);
      ellipse(center -windowWidth/4, windowHeight/2 -118, 120, 120);
      
      
     
      
      
    // Change color if fx value changed  
      
     noFill();     
	 if (fx === 1) {stroke(183, 147, 210);}
	else if (fx === 2) {stroke(136, 180, 220);}
	else if (fx === 2.5) {stroke(136, 180, 220);}
	else if (fx === 3) {stroke(125, 209, 159);}
	else if (fx === 4) {stroke(255, 202, 175);}
    strokeWeight(10);
        
         for (var i = 2; i < 3; i++) {
      const thickness = 15 * abs(encoder[i]);
      strokeWeight(thickness);
      ellipse(center +windowWidth/4, windowHeight/2 -118, 120, 120);

      
    for (var i = 3; i < 4; i++) {
      const thickness = 15 * abs(encoder[i]);
      strokeWeight(thickness);
      ellipse(center -windowWidth/4, windowHeight/2 +127, 120, 120);
     
          for (var i = 4; i < 5; i++) {
      const thickness = 15 * abs(encoder[i]);
      strokeWeight(thickness);
      ellipse(center +windowWidth/4, windowHeight/2 +127, 120, 120);
      
      
    }   
}
}
}
}

document.ontouchmove = function(event) {
 event.preventDefault();
};


