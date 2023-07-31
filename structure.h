void showstructure();
void saveDataInStructure(String structMineID, float structureLatitude, float structureLongitude, uint16_t structurevoltage, String packetTime, String structureStatus, int structureRssi, float structureSnr, char* structureRuntime) {
  savedData = false;
  for (byte t = 0; t < mineCount; t++) {
    if (mineqwerty[t].structId == structMineID) {
      mineqwerty[t].structStatus = structureStatus;
      mineqwerty[t].structLatitude = structureLatitude;
      mineqwerty[t].structLongitude = structureLongitude;
      strcpy(mineqwerty[t].structRuntime, structureRuntime);
      
      mineqwerty[t].structVoltage = structurevoltage;
      mineqwerty[t].structLastRecived = packetTime;
      mineqwerty[t].structRssi = structureRssi;
      mineqwerty[t].structSnr = structureSnr;
      savedData = true;
      break;
    }
  }
  if (savedData == false) {
    for (byte t = 0; t < mineCount; t++) {
      if (mineqwerty[t].structId == "0") {
        mineqwerty[t].structId = structMineID;
        mineqwerty[t].structStatus = structureStatus;

        mineqwerty[t].structLatitude = structureLatitude;
        mineqwerty[t].structLongitude = structureLongitude;
        strcpy(mineqwerty[t].structRuntime, structureRuntime);

        mineqwerty[t].structVoltage = structurevoltage;
        mineqwerty[t].structLastRecived = packetTime;
        mineqwerty[t].structRssi = structureRssi;
        mineqwerty[t].structSnr = structureSnr;
        break;
      }
    }
  }
}

/*
   true is to erase all data in structure
   false is erase specific cell of structure
*/

void eraseStructure(bool eraseState, String eraseID) {
  if (eraseState) {
    for (byte q = 0; q < mineCount; q++) {
      mineqwerty[q].structId = "0";
      mineqwerty[q].structStatus = "0";
      mineqwerty[q].structLatitude = 0.0;
      mineqwerty[q].structLongitude = 0.0;
      mineqwerty[q].structVoltage = 0;
      mineqwerty[q].structRssi = 0;
      mineqwerty[q].structSnr = 0;

    }
  } else {
    for (byte w = 0; w < mineCount; w++) {
      if (mineqwerty[w].structId == eraseID) {
        mineqwerty[w].structId = "0";
        mineqwerty[w].structStatus = "0";
        mineqwerty[w].structLatitude = 0.0;
        mineqwerty[w].structLongitude = 0.0;
        mineqwerty[w].structVoltage = 0;
        mineqwerty[w].structRssi = 0;
        mineqwerty[w].structSnr = 0;
        break;
        Serial.println("DEBUG");
      }
    }
  }
}

void showstructure() {
  Serial.println("--------------Structure start--------------");
  for (byte r = 0; r < mineCount; r++) {
    Serial.println("Structure number");
    Serial.println(mineqwerty[r].structId);
    Serial.println(mineqwerty[r].structLatitude);
    Serial.println(mineqwerty[r].structLongitude);
  }
  Serial.println("--------------Structure end--------------");
}
