#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include <SoftwareSerial.h>

Servo myservo;
int lastFingerID = -1; // Variabile per memorizzare l'ID dell'ultima impronta utilizzata

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(2, 3); // RX, TX per il sensore di impronte
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Indirizzo I2C e dimensioni dell'LCD

void setup()
{
  myservo.attach(9); 
  myservo.write(90); // Assicurati che il servo inizi dalla posizione chiusa

  Serial.begin(9600);
  lcd.init(); // Inizializza l'LCD con il numero di colonne e righe
  lcd.backlight();
  
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
    lcd.print("Sensore trovato!");
    delay(2000);
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    lcd.print("Sensore non trovato");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
    lcd.print("Nessuna impronta");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
    lcd.print("Pronto per l'uso");
  }
  pinMode(13, OUTPUT);
}

void loop() {
  if (lastFingerID == -1) {
    Serial.println("Waiting for a fingerprint to open the servo...");
    lcd.clear();
    lcd.print("Attesa impronta");
    getFingerprintID(); // Cerca di identificare un'impronta per aprire il servo
  } else {
    // Aspetta di riappoggiare la stessa impronta per chiudere il servo
    Serial.println("Waiting for the same fingerprint to close the servo...");
    lcd.clear();
    lcd.print(" Attesa stessa ");
    lcd.setCursor(0, 1);
    lcd.print("    impronta");
    if (getFingerprintIDez() == lastFingerID) {
      Serial.println("Fingerprint recognized, closing the servo...");
      lcd.clear();
      lcd.print("  Chiusura in");
      lcd.setCursor(0, 1);
      lcd.print("     corso");
      myservo.write(90); // Chiudi il servo (posizione iniziale)
      lastFingerID = -1; // Resetta l'ID dell'ultima impronta utilizzata
      delay(1500); // Attendi un po' prima di ripetere il loop
      Serial.println("Servo closed.");
      lcd.clear();
      lcd.print("    Chiusura");
      lcd.setCursor(0, 1);
      lcd.print("   effettuta");
      delay(1500);
    }
  }
  delay(50); // don't need to run this at full speed.
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      delay(500);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    lcd.clear();
    lcd.print("   Accesso non");
    lcd.setCursor(0, 1);
    lcd.print("   consentito");
    delay(2500);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  // Apri il servo
  Serial.println("Opening the servo...");
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Accesso");
  lcd.setCursor(3, 1);
  lcd.print("consentito");
  myservo.write(0); // Apri il servo
  lastFingerID = finger.fingerID; // Memorizza l'ID dell'impronta che ha aperto il servo
  delay(2000);
  Serial.println("Servo opened. Waiting to close...");
  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}
