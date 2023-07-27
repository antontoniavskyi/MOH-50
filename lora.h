#include <LoRa.h>

#define BAND 915E6

//ttgo LoRa pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

void initLoRa();

void loraSender(String jsonsender);
