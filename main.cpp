//
// INCLUDES
//

#include <Adafruit_NeoPixel.h>

//
// CONFIG
//

#define NEO_PIN   4
#define NUMPIXELS 125

#define B1_PIN	13
#define B2_PIN	12
#define R1_PIN	11
#define R2_PIN	10
#define P_PIN		9

#define NUM_PROGRAMS	7

byte program      = 0;
byte last_program = 0;

uint8_t param1 = 120;
uint8_t param2 = 120;

// 5 Buttons PIN|State|LastState
#define PIN           0
#define STATE         1
#define LAST_STATE    2

int buttons[5][3] = {
	{P_PIN,  0, LOW},
	{B1_PIN, 0, LOW},
	{B2_PIN, 0, LOW},
	{R1_PIN, 0, LOW},
	{R2_PIN, 0, LOW}
};

byte H = 0;
byte S = 0;
byte L = 0;

byte dimmedR = 0;
byte dimmedG = 0;
byte dimmedB = 0;

long lastDebounceTime[5] = {0};
long debounceDelay       = 50;
long lastLoopRun         = 0;

// LED Address Buffer: [Z|X|Y|RGB]
byte frameBuffer[5][5][5][3] = {0};
// LED BitMask Buffer: [Z|X|Y]
boolean frameMask[5][5][5] = {0};

Adafruit_NeoPixel cube = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

//Define readable names for colors
uint32_t red    = cube.Color(255, 0, 0);
uint32_t green  = cube.Color(0, 255, 0);
uint32_t blue   = cube.Color(0, 0, 255);
uint32_t yellow = cube.Color(128, 128, 0);
uint32_t white  = cube.Color(255, 255, 255);
uint32_t orange = cube.Color(255, 128, 0);
uint32_t purple = cube.Color(140, 0, 255);
uint32_t pink   = cube.Color(255, 80, 255);

void setup() {
	for (int i = 0; i < 5; ++i) {
		pinMode(buttons[i][PIN], INPUT);
	}
	cube.begin();
	cube.show();
	randomSeed(analogRead(0));
}

int counter = 0;
byte colorOffset = 0;

void loop() {
	checkButtons();

	switch(program) {
		case 0:
			rainbow_z(param1);
			last_program = 0;
		break;

		case 1:
			rainbowCycle(param1);
			last_program = 1;
		break;

		case 2:
			flickering(0, param1, 1);
			last_program = 2;
		break;

		case 3:
			flickering(Wheel(param2), param1, 0);
			last_program = 3;
		break;

		case 4:
			theaterChaseRainbow(param1);
			last_program = 4;
		break;

		case 5:
			for (int i = 0; i < 5; ++i) {
				shiftBufferUp();
			}
			runConway();
			last_program = 5;
		break;

		case 6:
			moodLight();
			last_program = 6;
		break;

		default:
			rain();
			last_program = 7;
		break;
	
	}
}

//
//	GENERATOR
//

// 

// raindrops [x|y|z|color]
#define N_DROPS 9
#define TAIL_LENGTH 4
#define RAIN_X  0
#define RAIN_Y  1
#define RAIN_Z  2
int raindrops[N_DROPS][3];
byte raindrops_color[N_DROPS];

void rain() {

	// init
	if (last_program != 7) {
		for (int i = 0; i < N_DROPS; ++i) {
			raindrops[i][RAIN_X] = random(0, 6);
			raindrops[i][RAIN_Y] = random(0, 6);
			raindrops[i][RAIN_Z] = random(0, 6 + TAIL_LENGTH);
			raindrops_color[i]	 = (byte) random(255);
		}
	}

	if ((millis() - lastLoopRun) > param1) {

		for (int i = 0; i < N_DROPS; ++i) {
			int z = raindrops[i][RAIN_Z];
			int x = raindrops[i][RAIN_X];
			int y = raindrops[i][RAIN_Y];
			uint32_t color = Wheel(raindrops_color[i]);
			byte r = Red(color);
			byte g = Green(color);
			byte b = Blue(color);

			for (int j = 0; j < TAIL_LENGTH; ++j) {
				if (z > -1 && z < 5) {
					frameBuffer[z][x][y][0] = r;
					frameBuffer[z][x][y][1] = g;
					frameBuffer[z][x][y][2] = b;
				}
				++z;

				//dimm(r, g, b, 200);
				//r = dimmedR;
				//g = dimmedG;
				//b = dimmedB;
			}

			// drop it...
			raindrops[i][RAIN_Z]--;

			if (raindrops[i][RAIN_Z] < -TAIL_LENGTH) {
				raindrops[i][RAIN_X] = random(0, 6);
				raindrops[i][RAIN_Y] = random(0, 6);
				raindrops[i][RAIN_Z] = 4;
				raindrops_color[i]	 = (byte) random(255);
			}

		}

		lastLoopRun = millis();
		showFrame();
		clearFrame();
	}
}


// WORKS!
void runConway() {
	// if first time running or after 5 generations
	if (last_program != 5 || counter > 4) {
		initConway();
		counter = 0;
	}

	if ((millis() - lastLoopRun) > param1) {
		conway();
		colorizeMask(param2 / 5, colorOffset);
		//gaussianFilter();
		showFrame();

		lastLoopRun = millis();
		counter++;
		colorOffset += 5;
	}
}

// WROKS!
void initConway() {
	for (int x = 0; x < 5; ++x) {
		for (int y = 0; y < 5; ++y) {
			frameMask[0][x][y] = (boolean) random(0, 2);
		}
	}
}

// WORKS!
void conway() {
	int neighbor[8];
	int num_neighbors = 0;
	int _x, _y;

	// Shift screen
	shiftMaskUp();

	// Wipe layer
	for (int x = 0; x < 5; ++x) {
		for (int y = 0; y < 5; ++y) {
			frameMask[0][x][y] = 0;
		}
	}

	// Calculate new layer
	for (int x = 0; x < 5; ++x) {
		for (int y = 0; y < 5; ++y) {

			// Calculate neighborhood
			neighbor[0] = frameMask[1][(x-1)%6][(y-1)%6];
			neighbor[1] = frameMask[1][(x-1)%6][y];
			neighbor[2] = frameMask[1][(x-1)%6][(y+1)%6];

			////////////////////////////////////////////

			neighbor[5] = frameMask[1][(x+1)%6][(y-1)%6];
			neighbor[6] = frameMask[1][(x+1)%6][y];
			neighbor[7] = frameMask[1][(x+1)%6][(y+1)%6];

			////////////////////////////////////////////

			neighbor[3] = frameMask[1][x][(y-1)%6];
			neighbor[4] = frameMask[1][x][(y+1)%6];

			// Sum up neighbors
			for (int i = 0; i < 8; ++i) {
				num_neighbors += neighbor[i];
			}

			// Under-population
			if ( frameMask[1][x][y] && (num_neighbors < 2) ) {

				frameMask[0][x][y] = 0;

			// Over-population
			} else if( frameMask[1][x][y] && (num_neighbors > 3 ) ) {

				frameMask[0][x][y] = 0;

			// Birth
			} else if( !frameMask[1][x][y] && (num_neighbors == 3) ) {

				frameMask[0][x][y] = 1;

			// Live another day...
			} else {

				frameMask[0][x][y] = frameMask[1][x][y];

			}

		}
	}
}

// WORKS!
byte moodLight_j = 0;

void moodLight() {
	if ((millis() - lastLoopRun) > 3000) {
		uint32_t color = Wheel(moodLight_j);
		++moodLight_j;
		for (int z = 0; z < 5; ++z) {
			for (int x = 0; x < 5; ++x) {
				for (int y = 0; y < 5; ++y) {
					frameBuffer[z][x][y][0] = Red(color);
					frameBuffer[z][x][y][1] = Green(color);
					frameBuffer[z][x][y][2] = Blue(color);
				}
			}
		}

		lastLoopRun = millis();
		//cube.show();
		showFrame();
	}
}

// WORKS!
byte rainbow_z_j = 0;

void rainbow_z(uint8_t wait) {
	if ((millis() - lastLoopRun) > wait) {
		for (int z = 0; z < 5; ++z) {
			uint32_t color = Wheel(rainbow_z_j + (z * 3) - param2);
			++rainbow_z_j;
			for (int x = 0; x < 5; ++x) {
				for (int y = 0; y < 5; ++y) {
					frameBuffer[z][x][y][0] = Red(color) - 5;
					frameBuffer[z][x][y][1] = Green(color) - 5;
					frameBuffer[z][x][y][2] = Blue(color) - 5;
				}
			}
		}

		lastLoopRun = millis();
		//cube.show();
		showFrame();
	}
}

// WORKS!
byte rainbow_j = 0;

void rainbow(uint8_t wait) {
	if ((millis() - lastLoopRun) > wait) {
		for(uint16_t i = 0; i < cube.numPixels(); i++) {
			//cube.setPixelColor(i, Wheel( (i + rainbow_j) & 255) );
			uint32_t color = Wheel( (i + rainbow_j) & 255);
			setPixel(i, Red(color), Green(color), Blue(color));
		}
		lastLoopRun = millis();
		++rainbow_j;
		//cube.show();
		showFrame();
	}
}

// WORKS!
byte rainbowCycle_j = 0;

void rainbowCycle(uint8_t wait) {
	if ((millis() - lastLoopRun) > wait) {
		for(uint16_t i = 0; i < cube.numPixels(); i++) {
			//cube.setPixelColor(i, Wheel(((i * 256 / cube.numPixels()) + rainbowCycle_j) & 255));
			uint32_t color = Wheel(((i * 256 / cube.numPixels()) + rainbowCycle_j) & 255);
			setPixel(i, Red(color), Green(color), Blue(color));
		}
		lastLoopRun = millis();
		++rainbowCycle_j;
		//cube.show();
		showFrame();
	}
}

// WORKS!
void flickering(uint32_t color, uint16_t delayt, boolean randcol) {
	//cube.clear();
	clearFrame();

	if ((millis() - lastLoopRun) > delayt) {
		uint16_t randpix = random(cube.numPixels());

		if(randcol == 1) color = randomColor();
		setPixel(randpix, Red(color), Green(color), Blue(color));
		//cube.setPixelColor(randpix, color);

		//cube.show();
		showFrame();
		lastLoopRun = millis();
	}
}

// WORKS!
byte theaterChaseRainbow_j = 0;
byte theaterChaseRainbow_q = 0;
boolean theaterChaseRainbow_on = false;

void theaterChaseRainbow(uint8_t wait) {
	if ((millis() - lastLoopRun) > wait) {

		if (!theaterChaseRainbow_on) {

			for (int i=0; i < cube.numPixels(); i=i+3) {
				//turn every third pixel on
				//cube.setPixelColor(i+theaterChaseRainbow_q, Wheel( (i+theaterChaseRainbow_j) % 255));
				uint32_t color = Wheel( (i+theaterChaseRainbow_j) % 255);
				setPixel(i+theaterChaseRainbow_q, Red(color), Green(color), Blue(color));
			}

			//cube.show();
			showFrame();

		} else {

			for (int i=0; i < cube.numPixels(); i=i+3) {
				//turn every third pixel off
				//cube.setPixelColor(i+theaterChaseRainbow_q, 0);
				setPixel(i+theaterChaseRainbow_q, 0, 0, 0);
			}

		}

		theaterChaseRainbow_on = !theaterChaseRainbow_on;
		lastLoopRun = millis();
		++theaterChaseRainbow_j;
		++theaterChaseRainbow_q;
		if (theaterChaseRainbow_q >= 3) theaterChaseRainbow_q = 0;

	}
}

//
//	FILTER
//

// WORKS!
void colorizeMask(int type, byte colorOffset) {
	uint32_t color;
	byte  k = 0;

	switch (type % 4) {
		// Color cycle each layer
		default:

			for (int z = 0; z < 5; ++z) {
				for (int y = 0; y < 5; ++y) {
					for (int x = 0; x < 5; ++x) {
						if ( frameMask[z][x][y] == 1 ) {
							color = Wheel(k + colorOffset);
							frameBuffer[z][x][y][0] = Red(color);  // R
							frameBuffer[z][x][y][1] = Green(color);// G
							frameBuffer[z][x][y][2] = Blue(color); // B
						} else {
							frameBuffer[z][x][y][0] = 0; // R
							frameBuffer[z][x][y][1] = 0; // G
							frameBuffer[z][x][y][2] = 0; // B
						}
					}
				}
				++k;
			}

		break;
		// Color cycle each LED
		case 1:

			for (int z = 0; z < 5; ++z) {
				for (int y = 0; y < 5; ++y) {
					for (int x = 0; x < 5; ++x) {
						if ( frameMask[z][x][y] == 1 ) {
							color = Wheel(k + colorOffset);
							++k;
							frameBuffer[z][x][y][0] = Red(color);  // R
							frameBuffer[z][x][y][1] = Green(color);// G
							frameBuffer[z][x][y][2] = Blue(color); // B
						} else {
							frameBuffer[z][x][y][0] = 0; // R
							frameBuffer[z][x][y][1] = 0; // G
							frameBuffer[z][x][y][2] = 0; // B
						}
					}
				}
			}

		break;

		// Random color
		case 2:

		for (int z = 0; z < 5; ++z) {
				for (int y = 0; y < 5; ++y) {
					for (int x = 0; x < 5; ++x) {
						if ( frameMask[z][x][y] == 1 ) {
							color = Wheel( (byte) random(0, 256) );
							frameBuffer[z][x][y][0] = Red(color);  // R
							frameBuffer[z][x][y][1] = Green(color);// G
							frameBuffer[z][x][y][2] = Blue(color); // B
						} else {
							frameBuffer[z][x][y][0] = 0; // R
							frameBuffer[z][x][y][1] = 0; // G
							frameBuffer[z][x][y][2] = 0; // B
						}
					}
				}
			}
		
		break;
		
		// Solid fill
		case 3:

		for (int z = 0; z < 5; ++z) {
			for (int y = 0; y < 5; ++y) {
				for (int x = 0; x < 5; ++x) {
					if ( frameMask[z][x][y] == 1 ) {
						color = Wheel(colorOffset);
						frameBuffer[z][x][y][0] = Red(color);  // R
						frameBuffer[z][x][y][1] = Green(color);// G
						frameBuffer[z][x][y][2] = Blue(color); // B
					} else {
						frameBuffer[z][x][y][0] = 0; // R
						frameBuffer[z][x][y][1] = 0; // G
						frameBuffer[z][x][y][2] = 0; // B
					}
				}
			}
		}

		break;

	}
}

// ALL TURNS BLEACK :/
// Maybe unstable due to overflows or casting errors?
void gaussianFilter() {
	float coef = 0.333;
	byte color1[3], color2[3], color3[3], sum[3];

	// y dimension
	for (int z = 0; z < 5; ++z) {
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 5; ++y) {

				if (y != 0) {
					color1[0] = frameBuffer[z][x][y-1][0]; // R
					color1[1] = frameBuffer[z][x][y-1][1]; // G
					color1[2] = frameBuffer[z][x][y-1][2]; // B
				} else {
					color1[0] = 1;
					color1[1] = 1;
					color1[2] = 1;
				}

				color2[0] = frameBuffer[z][x][y][0]; // R
				color2[1] = frameBuffer[z][x][y][1]; // G
				color2[2] = frameBuffer[z][x][y][2]; // B

				if (y < 5) {
					color3[0] = frameBuffer[z][x][y+1][0]; // R
					color3[1] = frameBuffer[z][x][y+1][1]; // G
					color3[2] = frameBuffer[z][x][y+1][2]; // B
				} else {
					color3[0] = 1;
					color3[1] = 1;
					color3[2] = 1;
				}

				for (int i = 0; i < 3; ++i) {
					color1[i] = (byte) (color1[i] * coef);
					color2[i] = (byte) (color2[i] * coef);
					color3[i] = (byte) (color3[i] * coef);
				}

				for (int i = 0; i < 3; ++i) {
					color1[i] = (byte) (color1[i] * coef);
					color2[i] = (byte) (color2[i] * coef);
					color3[i] = (byte) (color3[i] * coef);
				}

				for (int i = 0; i < 3; ++i) {
					if ( (color1[i] + color2[i] + color3[i]) < 255) {
						sum[i] = color1[i] + color2[i] + color3[i];
					} else {
						sum[i] = 255;
					}
					
				}

				frameBuffer[z][x][y][0] = sum[0]; // R
				frameBuffer[z][x][y][1] = sum[1]; // G
				frameBuffer[z][x][y][2] = sum[2]; // B

			}
		}
	}

	// x dimension

	for (int z = 0; z < 5; ++z) {
		for (int y = 0; y < 5; ++y) {
			for (int x = 0; x < 5; ++x) {

				if (x != 0) {
					color1[0] = frameBuffer[z][x-1][y][0]; // R
					color1[1] = frameBuffer[z][x-1][y][1]; // G
					color1[2] = frameBuffer[z][x-1][y][2]; // B
				} else {
					color1[0] = 1;
					color1[1] = 1;
					color1[2] = 1;
				}

				color2[0] = frameBuffer[z][x][y][0]; // R
				color2[1] = frameBuffer[z][x][y][1]; // G
				color2[2] = frameBuffer[z][x][y][2]; // B

				if (x < 5) {
						color3[0] = frameBuffer[z][x+1][y][0]; // R
						color3[1] = frameBuffer[z][x+1][y][1]; // G
						color3[2] = frameBuffer[z][x+1][y][2]; // B
				} else {
					color3[0] = 1;
					color3[1] = 1;
					color3[2] = 1;
				}

				for (int i = 0; i < 3; ++i) {
					color1[i] = (byte) (color1[i] * coef);
					color2[i] = (byte) (color2[i] * coef);
					color3[i] = (byte) (color3[i] * coef);
				}

				for (int i = 0; i < 3; ++i) {
					color1[i] = (byte) (color1[i] * coef);
					color2[i] = (byte) (color2[i] * coef);
					color3[i] = (byte) (color3[i] * coef);
				}

				for (int i = 0; i < 3; ++i) {
					if ( (color1[i] + color2[i] + color3[i]) < 255) {
						sum[i] = color1[i] + color2[i] + color3[i];
					} else {
						sum[i] = 255;
					}
					
				}

				frameBuffer[z][x][y][0] = sum[0]; // R
				frameBuffer[z][x][y][1] = sum[1]; // G
				frameBuffer[z][x][y][2] = sum[2]; // B

			}
		}
	}

	// z dimension

	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 5; ++x) {
			for (int z = 0; z < 5; ++z) {

				if (z != 0) {
					color1[0] = frameBuffer[z-1][x][y][0]; // R
					color1[1] = frameBuffer[z-1][x][y][1]; // G
					color1[2] = frameBuffer[z-1][x][y][2]; // B
				} else {
					color1[0] = 1;
					color1[1] = 1;
					color1[2] = 1;
				}
				color2[0] = frameBuffer[z][x][y][0]; // R
				color2[1] = frameBuffer[z][x][y][1]; // G
				color2[2] = frameBuffer[z][x][y][2]; // B

				if (z < 5) {
					color3[0] = frameBuffer[z+1][x][y][0]; // R
					color3[1] = frameBuffer[z+1][x][y][1]; // G
					color3[2] = frameBuffer[z+1][x][y][2]; // B
				} else {
					color3[0] = 1;
					color3[1] = 1;
					color3[2] = 1;
				}
				
				for (int i = 0; i < 3; ++i) {
					color1[i] = (byte) (color1[i] * coef);
					color2[i] = (byte) (color2[i] * coef);
					color3[i] = (byte) (color3[i] * coef);
				}

				for (int i = 0; i < 3; ++i) {
					color1[i] = (byte) (color1[i] * coef);
					color2[i] = (byte) (color2[i] * coef);
					color3[i] = (byte) (color3[i] * coef);
				}

				for (int i = 0; i < 3; ++i) {
					if ( (color1[i] + color2[i] + color3[i]) < 255) {
						sum[i] = color1[i] + color2[i] + color3[i];
					} else {
						sum[i] = 255;
					}
					
				}

				frameBuffer[z][x][y][0] = sum[0]; // R
				frameBuffer[z][x][y][1] = sum[1]; // G
				frameBuffer[z][x][y][2] = sum[2]; // B

			}
		}
	}
}

//
//	UTILITY
//

// Untested
void shiftBufferUp() {
	for (int z = 5; z > 5; --z) {
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 5; ++y) {
				for (int k = 0; k < 3; ++k) {
					frameBuffer[z][x][y][k] = frameBuffer[z-1][x][y][k];
				}
			}
		}
	}
}

// WORKS!
void shiftMaskUp() {
	for (int z = 5; z > 0; --z) {
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 5; ++y) {
				frameMask[z][x][y] = frameMask[z-1][x][y];
			}
		}
	}
}

// WORKS!
void shiftMaskDown() {
	for (int z = 0; z < 4; ++z) {
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 5; ++y) {
				frameMask[z][x][y] = frameMask[z+1][x][y];
			}
		}
	}
}

// WORKS!
void setPixel(int index, byte r, byte g, byte b) {
	int z = index / (25);
	int x = (index / 5) % 5;
	int y = index % 5;

	frameBuffer[z][x][y][0] = r;
	frameBuffer[z][x][y][1] = g;
	frameBuffer[z][x][y][2] = b;
}

// WORKS!
void clearFrame() {
	for (int z = 0; z < 5; ++z) {
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 5; ++y) {
				for (int k = 0; k < 3; ++k) {
					frameBuffer[z][x][y][k] = 0;
				}
			}
		}
	}
}

// WORKS!
void showFrame() {
	int i = 0;
	byte tmp[3] = {0};
	
	// Read frame buffer and switch first and last two LEDs on every other row
	for (int z = 0; z < 5; ++z) {
		for (int x = 0; x < 5; ++x) {

			if ( (x % 2) != 0 ) { // On every other row...
				// Swap first and last LED
				for (int k = 0; k < 3; ++k) {
					tmp[k] = frameBuffer[z][x][0][k];
					frameBuffer[z][x][0][k] = frameBuffer[z][x][4][k];
					frameBuffer[z][x][4][k] = tmp[k];
				}
				//Swap second and second to last LED
				for (int k = 0; k < 3; ++k) {
					tmp[k] = frameBuffer[z][x][1][k];
					frameBuffer[z][x][1][k] = frameBuffer[z][x][3][k];
					frameBuffer[z][x][3][k] = tmp[k];
				}
			}

			for (int y = 0; y < 5; ++y) {

				cube.setPixelColor(i, cube.Color(
					frameBuffer[z][x][y][0], // R
					frameBuffer[z][x][y][1], // G
					frameBuffer[z][x][y][2]  // B
				));
				i++;
			}

			//Swap back
			if ( (x % 2) != 0 ) { // On every other row...
				// Swap first and last LED
				for (int k = 0; k < 3; ++k) {
					tmp[k] = frameBuffer[z][x][4][k];
					frameBuffer[z][x][4][k] = frameBuffer[z][x][0][k];
					frameBuffer[z][x][0][k] = tmp[k];
				}
				//Swap second and second to last LED
				for (int k = 0; k < 3; ++k) {
					tmp[k] = frameBuffer[z][x][3][k];
					frameBuffer[z][x][3][k] = frameBuffer[z][x][1][k];
					frameBuffer[z][x][1][k] = tmp[k];
				}
			}
		}
	}

	cube.show();
}

// WORKS!
void checkButtons() {

	for (int i = 0; i < 5; ++i) {
		int reading = digitalRead(buttons[i][PIN]);

		if (reading != buttons[i][LAST_STATE]) {
			lastDebounceTime[i] = millis();
		} 
		
		if ((millis() - lastDebounceTime[i]) > debounceDelay) {
			// if the button state has changed
			if (reading != buttons[i][STATE]) {
				buttons[i][STATE] = reading;
				
				if (buttons[i][STATE] == HIGH) {
					switch(i) {
						case 1:
							param1 -= 10;
						break;

						case 2:
							param1 += 10;
						break;

						case 3:
							param2 -= 10;
						break;

						case 4:
							param2 += 10;
						break;

						default:
							++program;
							if (program > NUM_PROGRAMS) program = 0;
						break;
					}
					
				}
			}
		}

		buttons[i][LAST_STATE] = reading;
	}
}

// WORKS!
uint32_t randomColor() {
	uint8_t r, g, b;

	r = random(255);
	g = random(255);
	b = random(255);

	uint32_t randcol = cube.Color(g, r, b);

	return randcol;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
	WheelPos = 255 - WheelPos;
	if(WheelPos < 85) {
		return cube.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	if(WheelPos < 170) {
		WheelPos -= 85;
		return cube.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
	WheelPos -= 170;
	return cube.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color) {
	return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color) {
	return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color) {
	return color & 0xFF;
}

// Loosing Hue or Saturation during conversion...?
void RGBtoHSL(int r, int g, int b) {
	r /= 255;
	g /= 255;
	b /= 255;

	int max = max(max(r, g), b);
	int min = min(min(r, g), b);

	int h = (max + min) / 2;
	int s = h;
	int l = h;

	if (max == min) {
		h = 0;
		s = 0; // achromatic
	} else {
		int d = max - min;
		s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

		if (max == r) {
			h = (g - b) / d + (g < b ? 6 : 0);
		} else if (max == g) {
			h = (b - r) / d + 2;
		} else if (max == b) {
			h = (r - g) / d + 4;
		}

		h /= 6;

		H = (byte) round(h);
		S = (byte) round(s);
		L = (byte) round(l);
	}
}

int hue2rgb(int p, int q, int t){
	if(t < 0) ++t;
	if(t > 1) --t;
	if(t < 1/6) return p + (q - p) * 6 * t;
	if(t < 1/2) return q;
	if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;
	return p;
}

void HSLtoRGB(int h, int s, int l) {
	int r, g, b;

	if(s == 0){
		r = l;
		g = l;
		b = l; // achromatic
	}else{
		int q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		int p = 2 * l - q;
		r = hue2rgb(p, q, h + 1/3);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1/3);
	}

	dimmedR = (byte) round(r * 255);
	dimmedG = (byte) round(g * 255);
	dimmedB = (byte) round(b * 255);
}

void dimm(byte r, byte g, byte b, byte amount) {
	RGBtoHSL(r, g, b);
	L -= amount;
	if (L < 0) L = 0;
	HSLtoRGB(H, S, L);
}