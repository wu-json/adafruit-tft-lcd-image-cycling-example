#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ST7735.h>      // Hardware-specific library
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions

// TFT display and SD card share the hardware SPI interface, using
// 'select' pins for each to identify the active device on the bus.
#define SD_CS    4 // SD card select pin
#define TFT_CS  10 // TFT select pin
#define TFT_DC   8 // TFT display/command pin
#define TFT_RST  9 // Or set to -1 and connect to Arduino RESET pin

#define MAX_IMAGE_COUNT 10        // Max images to load.
#define FILENAME_CHAR_LIMIT 25    // Max filename lenght for images.

SdFat                SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys
Adafruit_ST7735      tft    = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

int             images_loaded = 0;                               // How many images were loaded
int             image_to_show_index = 0;                         // Which image is currently being shown
int             time_between_photos_ms = 10000;                  // How long to wait between photos
char            images[MAX_IMAGE_COUNT][FILENAME_CHAR_LIMIT];    // Array of image filenames to cycle through

// Takes images from the SD card and stores them in the global images variable.
// Also return the total number of images loaded.
int loadImages(File dir, char images[MAX_IMAGE_COUNT][FILENAME_CHAR_LIMIT]) {
  int images_index = 0;

  while(true) {
    File file = dir.openNextFile();
    if (!file) {
      break;
    }

    char name[25];
    file.getName(name, 25);

    char *dot = strrchr(name, '.');
    if (dot && !strcmp(dot, ".bmp") && name[0] != '.') {
      Serial.print(name);
      Serial.print(": loaded image ");
      Serial.println(images_index);
      strcpy(images[images_index], name);
      images_index++;
    } 

    file.close();
  }

  return images_index;
}

void setup() {
  File root; // Root directory

  Serial.begin(9600);
  while(!Serial);       // Wait for Serial Monitor before continuing

  tft.initR(INITR_144GREENTAB); // Initialize screen
  tft.setRotation(3);

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatVolume object. This MUST
  // BE INITIALIZED before using any of the image reader functions!
  Serial.print(F("Initializing filesystem..."));

  // SD card is pretty straightforward, a single call...
  if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
    Serial.println(F("SD begin() failed"));
    for(;;); // Fatal error, do not continue
  }

  Serial.println(F("OK!"));

  // Fill screen blue. Not a required step, this just shows that we're
  // successfully communicating with the screen.
  tft.fillScreen(ST7735_BLUE);

  // Let's load all of the bmp image file names we find in the root of 
  // the SD card into the images array.
  Serial.println("Searching for .bmp images...");
  Serial.println("---");
  root = SD.open("/");
  images_loaded = loadImages(root, images);
  Serial.println("---");

  Serial.print("Images loaded: ");
  Serial.println(images_loaded);
  Serial.println("---");

  Serial.println("Complete...starting loop");
  delay(2000);
}

void loop() {
  if (images_loaded == 0) {
    Serial.println("No images to display");
    exit(1);
    return;
  }

  ImageReturnCode stat;

  char image_path[FILENAME_CHAR_LIMIT];
  snprintf(image_path, 100, "/%s", images[image_to_show_index]);

  Serial.print("Drawing image - ");
  Serial.print(image_path);
  Serial.print("...");

  stat = reader.drawBMP(images[image_to_show_index], tft, 0, 0);
  reader.printStatus(stat);

  if (image_to_show_index == images_loaded - 1) {
    image_to_show_index = 0;
  } else {
    image_to_show_index++;
  }

  delay(time_between_photos_ms);
}
