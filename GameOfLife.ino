//http://cogprints.org/4115/1/CollisionBasedRennard.pdf
//http://hypnocube.com/2013/12/design-and-implementation-of-serial-led-gadgets/
//https://stackoverflow.com/questions/5612174/more-ram-efficient-boolean-array-arduino-environment

#include "FastLED.h"

#include <TrueRandom.h>
// Hungry hungry ram whore

#define DEBUG true
/*
 This is an example of how simple driving a Neopixel can be
 This code is optimized for understandability and changability rather than raw speed
 More info at http://wp.josh.com/2014/05/11/ws2812-neopixels-made-easy/
*/

// Change this to be at least as long as your pixel string (too long will work fine, just be a little slower)


// These values depend on which pin your string is connected to and what board you are using 
// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation

// These values are for digital pin 8 on an Arduino Yun or digital pin 12 on a DueMilinove
// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.


#define ledWidth 5
#define ledHeight 5
#define colourDecay 8
#define NUM_LEDS (ledWidth * ledHeight)
#define saturation 255

#define LED_PIN     10
#define BRIGHTNESS  96
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

int generationCounter;
int pixelCounter = 0;
int deadCounter = 0;

int numberofLivingCells = 0;

CRGB RGBArray[ledWidth*ledHeight];

struct HSV {
  uint8_t h;
  uint8_t v;
}; 

//This struct holds our colour and fade values
HSV array[ledWidth][ledHeight];

//This uint32_t array holds our boolean bitmasked values for the next generation
HSV nextGenArray[ledWidth][ledHeight];

//0|0|0
//-----
//0|X|0
//-----
//0|0|0

//We can bitshift each of the values accordingly
uint8_t aliveCheck = B00011100;
uint8_t deadCheck = B00010010;    

//Timer Stuff
double lastTime;
double now;
double ns     = 1000 / 0.5; //How many times a second does the cycle run, 0.5fps currently.
double delta  = 0;
int    tmpCnt = 0;



void setup() {   
  Serial.begin(115200);
   Serial.println("Begin");
  lastTime = millis();
  LEDS.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(RGBArray,NUM_LEDS);
  LEDS.setBrightness(BRIGHTNESS);
  pinMode( 10 , OUTPUT );
  //Flash the entire string for debugging
  showColor(100,255,0);
  if(DEBUG)showColor(100,100,100); delay(100);
  if(DEBUG)showColor(0,0,0);
  initGameBoard();
  if(DEBUG)gameModes(TrueRandom.random(25));
  //if(DEBUG)gameModes(13);
}


void loop() {
  now = millis();
  delta = delta + (now-lastTime) / ns;
  lastTime = now;
  
  
  
  while (delta >= 1)  {
    //DoStuff 1fps
//    computeDecay();
    nextGeneration(); //Calculate the next generation of LED values
    delta--; 
    sendXY(); //Prepare the outbound array
    showData();
  }
}

// Display a single color on the whole string

void showColor( unsigned char r , unsigned char g , unsigned char b ) {  
  for( int p=0; p<NUM_LEDS; p++ ) {
    RGBArray[p] = CRGB(r, g, b);
  }
  Serial.print("Showing Colour :");
  Serial.print("r ");
  Serial.print(r);
  Serial.print(" g ");
  Serial.print(g);
  Serial.print(" b ");
  Serial.println(b);
  FastLED.show(); 
}
//Prepare the outbound array and send out the data
void showData() {   
  //Why was I writing values directly from the array... Silly
  //for( int p=0; p<NUM_LEDS; p++ ) {
  //   RGBArray[p] = CRGB( RGBArray[p].r , RGBArray[p].g , RGBArray[p].b );
  //}
  Serial.println("Sending data");
  FastLED.show(); 
}

//This function prepares the array for outbound LED transit as the HSI to RGB Colouspace conversion takes longer than the available latch time to send the data stream to be queued down the data line for the WS2811 LEDs.
void sendXY(){
  pixelCounter = 0;
  for(int x = 0; x < ledWidth; x++) {
    for(int y = 0; y < ledHeight; y++) {    
//        Serial.print("Hue ");Serial.print(array[x][y].h);Serial.print("Value ");Serial.print(array[x][y].v); Serial.print("Position ");Serial.print(XY(x,y)); 
        hsi2rgb(array[x][y].h, saturation, array[x][y].v, XY(x,y)); 
        if(DEBUG)Serial.print(" H "); Serial.print(array[x][y].h); Serial.print(" V "); Serial.print(array[x][y].v);   
      }   
      if(DEBUG)Serial.println();
    }  
//Serial.print("Pixel Counter : "); Serial.println(pixelCounter);
}

// Helper function that translates from x, y into an index into the LED array
// Handles both 'row order' and 'serpentine' pixel layouts.
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (ledWidth - 1) - x;
      i = (y * ledWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * ledWidth) + x;
    }
  
  return i;
}



void hsi2rgb(float H, float S, float I, int j) {
  byte r, g, b;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
    
  // Math! Thanks in part to Kyle Miller.
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }
  RGBArray[j].r = r;
  RGBArray[j].g = g;
  RGBArray[j].b = b;
}



void initGameBoard() {
  //Loop through the neighbouring cells and get their hue
  int seed = 0;
  for (uint16_t i = 0; i < ledWidth; i++) {
    for (uint16_t j = 0; j < ledHeight; j++) {
      seed = TrueRandom.random(400);
      //array[i][j] = {0, 0};
      if(seed > 255) { 
        array[i][j].v = 255;
      } else {
        array[i][j].v = 0;
      }
      array[i][j].h = TrueRandom.random(255);
      //if(array[i][j].v > 255) {
      //  array[i][j].v = 255;
      //}
      Serial.print(array[i][j].v); Serial.print(",");
    }
    Serial.println();
  }
}


//Compute the next generation of the game board
void nextGeneration() {

  generationCounter++;
  numberofLivingCells = 0;
  deadCounter = 0;
  for (uint16_t yy = 0; yy < ledHeight; yy++) {
    for (uint16_t xx = 0; xx < ledWidth; xx++) {
        if(checkIfAliveOrDead(xx,yy) == true){
          nextGenArray[xx][yy].v = 255;
          numberofLivingCells++;
        } else {
          nextGenArray[xx][yy].v =  nextGenArray[xx][yy].v - colourDecay;
          deadCounter++;
        }
        //Now we blend the colours if value is not black
        //Yay polar coordinates to the rescue....
        if(array[xx][yy].v > 0) {
          //Send our current position and Hue to the Hue averager, place in nextGenArray
          nextGenArray[xx][yy].h = averageHue(xx, yy, array[xx][yy].h);
        }
//       Serial.print(array[xx][yy].h); Serial.print(",");
    }  
//    Serial.println();
  } 
  Serial.print("Generation ");
  Serial.print(generationCounter);
  Serial.print(" Number of cells alive ");
  Serial.print(numberofLivingCells);
  Serial.print(" Number of cells killed ");
  Serial.println(deadCounter);
  if(numberofLivingCells <= 0) { 
    Serial.println("All cells are dead, resetting");
    Serial.print("Number of alive cells ");
    Serial.println(numberofLivingCells);
    showColor(0,255,0);
    initGameBoard();
  }
  
  //Finally we memcpy the array to the primary array
  memcpy(array, nextGenArray, sizeof(array));

}
/*
void computeDecay(){
  //For each pixel
  if(DEBUG)Serial.println("Computing decay");
  
  for (uint16_t yy = 0; yy < ledWidth; yy++) {
    for (uint16_t xx = 0; xx < ledHeight; xx++) {
      //If our pixel isnt fully on or nearly off, then we can begin to decay the colour
      if( nextGenArray[xx][yy].v >= colourDecay && nextGenArray[xx][yy].v < 254){
            nextGenArray[xx][yy].v = array[xx][yy].v - colourDecay;
          } else {
            //If we're too close to the bound, instead of looping around, write zero
            nextGenArray[xx][yy].v = 0;  
          }
    }
  }
  
  memcpy(array, nextGenArray, sizeof(array));
}
*/


uint8_t averageHue(int16_t x, int16_t y, uint8_t H){
  // 3x3 pixel filter kernel, initalized with the current Hue so edges dont show only as red, Hue = 0
  uint8_t Hue[9] = {H,H,H,H,H,H,H,H,H};
  int8_t counter = 0;
  
  for (int8_t yy = -1; yy <= 1; yy++) { 
    for (int8_t xx = -1; xx <= 1; xx++) {
      if (x + xx >= 0 && y + yy >= 0 && x + xx <= ledWidth && y + yy <= ledHeight) {//Boundary check
         if(array[x+xx][y+yy].v =! 0){//if our neighbour is off, then use our current cells hue. Already set in the first line of this function
           Hue[counter] = array[x+xx][y+yy].h; //Extract the hue from each position relative to the kernel position
         }
         //Serial.print(x+xx); Serial.print(",");Serial.print(y+yy);Serial.print(":");Serial.print(Hue[counter]);Serial.print(" ");
         counter++;
        
      }
      
    } 
   //Serial.println();
  }
   
  //HSL averaging
  //https://stackoverflow.com/questions/8169654/how-to-calculate-mean-and-standard-deviation-for-hue-values-from-0-to-360
    //https://en.wikipedia.org/wiki/Mean_of_circular_quantities
  //https://stackoverflow.com/questions/13959276/averaging-circular-values-particularly-hues-in-hsl-color-scheme
  //Now we filter the 3x3 image kernel
  float X = 0.0f;
  float Y = 0.0f;
  for (int8_t i = 0; i < 9; i++){
//    Serial.print("H");Serial.print((float)Hue[i]);Serial.print(",");   
    X += cos((float)Hue[i] / 180 * PI);
//     Serial.print("x");Serial.print(X);Serial.print(",");   
    Y += sin((float)Hue[i] / 180 * PI);
//      Serial.print("y,");Serial.print(Y);Serial.println(",");   
  }
    //Now average the X and Y values
    X /= 9;
    Y /= 9;
    //Get atan2 of those
    float val = atan2(Y, X) * 180 / PI; //NB, atan2 takes its arguments reversed, is actually shorthand for atan(Y / X)
//    Serial.print(val);Serial.print(",");   
//    Serial.println();
    return val;
  
}



boolean checkIfAliveOrDead(uint16_t x, uint16_t y) {
  //Get our return value, compare the cells neighbour count against the alive and dead states and return weather the cell lives or dies
  boolean currentState; //Is our current cell alive or dead
  //alive is equal to value == 255
  int val = getNeighbours(x, y);
  if (array[x][y].v == 255) { //If our current cell is alive then currentState == TRUE
    currentState = true;
  } else {
    currentState = false;
  }
  

  //Serial.print("n");Serial.print(val); Serial.print(":");
  if (currentState == true) {
    //Every uint8_t (unsigned char) will contain 8 boolean values, to get the i-th you can just use values & (1 << i) in if tests, and it will work since if the correct bit is set, then result will be != to 0.
    if (bitRead(aliveCheck, val) == 1) { //If our value returns 1 we keep the current cell
      Serial.println("AA");
      return true;
    } else {
      //Serial.print("AD");
      return false;
    }
  } else { //If our cell is currently dead, we see if the condition is met to bring it back to life
    if (bitRead(deadCheck, val) == 1) {//Come back to life
      return true;
    } else {
      return false;
    }
  }
}

//Check the number neighbors alive, out of a possible 9 values we can deduce the number of neighbouring states and compare to the
//two declared structs and determine our behavour accordingly

char getNeighbours(int x, int y) {
    Serial.print("X "); Serial.print(x);
  Serial.print(" Y "); Serial.println(y);

  int val = 0; //Neighbour cell count value holder
  //Loop through the neighbouring cells and see if they're alive
  for (int yy = -1; yy <= 1; yy++) { //Char is -127 through 127, trying int for datatype uniformity....
    for (int xx = -1; xx <= 1; xx++) {
      //if we're outside of the array, eg 0 + -1 would equal -1 and is out of bounds, then do nothing
      if (x + xx >= 0 && y + yy >= 0 && x + xx <= ledWidth && y + yy <= ledHeight) {
      Serial.print(" Pos X "); Serial.print(xx + x); Serial.print(" ");Serial.print(xx);
      Serial.print(" Pos Y "); Serial.print(yy + y); Serial.print(" ");Serial.print(yy);
      Serial.print(" v ");Serial.print(array[x + xx][y + yy].v);
//         if(x+xx != x && y + yy != y){//Lets not count ourselves
        //We get our current pointer value and if the value is max add one to the live neighbor count
          
          if (array[x + xx][y + yy].v == 255) { //alive is equal to value == 255
            val++;
            Serial.print("+");
          }
        }
//      }
    }Serial.println();
  }Serial.println();
  Serial.print(" Neighbours "); Serial.println(val);
  return val;
}











void gameModes(int var){

//Template for different rules with bit position, use insert to replace with 1's and 0's according to the rule set  
//uint8_t aliveCheck = B87654321;
//uint8_t deadCheck = B87654321;
  
switch(var) {
  case 1:
    Serial.println("Dry Life '23/37'");
    aliveCheck = B00000110;
    deadCheck = B01000100;
    break;
  
  case 2:
    Serial.println("2x2 '125/36'");
    aliveCheck = B00010011;
    deadCheck = B00100100;
    break;
  
  case 3:
    Serial.println("3x4  '34/34'");
    aliveCheck = B00001100;
    deadCheck = B00001100;
    break;
  
  case 4:
    Serial.println("Amoeba '1358/357'");
    aliveCheck = B10010101;
    deadCheck = B01010100;
    break;
  
  case 5:
    Serial.println("Assimilation '4567/345'");
    aliveCheck = B01111000;
    deadCheck = B00011100;
    break;
  
  case 6:
    Serial.println("Coagulations '235678/378'");
    aliveCheck = B11110110;
    deadCheck = B11000100;
    break;
              
  case 7:            
    Serial.println("Conway\'s Life', '23/3'");   
    aliveCheck = B00000110;
    deadCheck = B00000100;
    break;
  
  case 8:            
    Serial.println("'Coral', '45678/3'");
    aliveCheck = B11111000;
    deadCheck = B00000100;   
    break;    
        
  case 9:            
    Serial.println("'Day & Night','34678/3678'");
    aliveCheck = B11101100;
    deadCheck = B11100100;
    break;
  
  case 10:
    Serial.println("'Diamoeba', '5678/35678'");
    aliveCheck = B11110000;
    deadCheck = B11110100;
    break;
              
  case 11:
    Serial.println("'Gnarl', '1/1'");
    aliveCheck = B00000001;
    deadCheck = B00000001;
    break;
         
  case 12:
    Serial.println("'High Life', '23/36'");
    aliveCheck = B00000110;
    deadCheck = B00100100;
    break;
  
  case 13:
    Serial.println("'Long life', '5/345'");
    aliveCheck = B00010000;
    deadCheck = B00011100;
    break;
  
  case 14:
    Serial.println("'Maze', '12345/3'");
    aliveCheck = B00011111;
    deadCheck = B00000100;
    break;
  
  case 15:
    Serial.println("'Mazectric', '1234/3'");
    aliveCheck = B00001111;
    deadCheck = B00000100;
    break;
  
  case 16:
    Serial.println("'Move', '245/368'");
    aliveCheck = B00011010;
    deadCheck = B10100100;
    break;
    
  case 17:
    Serial.println("'Pseudo life','238/357'");
    aliveCheck = B10000110;
    deadCheck = B01010100;
    break;
    
  case 18:
    Serial.println("'Replicator', '1357/1357'");
    aliveCheck = B01010101;
    deadCheck = B01010101;
    break;
    
  case 19:
    Serial.println("'Seeds',rule: '/2'");
    aliveCheck = B00000000;
    deadCheck = B00000010;
    break;
    
  case 20:
    Serial.println("'Serviettes', '/234'");
    aliveCheck = B00000000;
    deadCheck = B00001110;
    break;
    
  case 21:
    Serial.println("'Stains', '235678/3678'");
    aliveCheck = B11110110;
    deadCheck = B11100100;
    break;
    
  case 22:
    Serial.println("'Vote', '45678/5678'");
    aliveCheck = B11111000;
    deadCheck = B11110000;
    break;
    
  case 23:
    Serial.println("'Vote 4/5', '45678/5678'");
    aliveCheck = B11111000;
    deadCheck = B11110000;
    break;
    
  case 24:
    Serial.println("'Walled Cities', '2345/45678'");
    aliveCheck = B00011110;
    deadCheck = B11111000;
    break;
  }
}
