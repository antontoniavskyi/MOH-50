#include <TinyGPS++.h>


float latitude; //Defined here, because compiler show redefinition error
float longitude; //Defined here, because compiler show redefinition error

// The TinyGPS++ object
TinyGPSPlus gps; //Defined here, because compiler show redefinition error


#if defined(tbeam)
void gpsRequest() {
  latitude = 0;
  longitude = 0;
  while (Serial1.available() > 0)
    if (gps.encode(Serial1.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
    }
#ifdef DEBUG
  Serial.println("GPS STATUS START");
  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("Longtitude: ");
  Serial.println(longitude, 6);
  Serial.println("GPS STATUS END");
#endif
}
#else
void gpsRequest() {}
#endif
//void gpsRequest();
