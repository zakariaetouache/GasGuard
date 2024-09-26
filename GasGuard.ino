#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal.h>
#include <ESP32Servo.h>
#include "config.h"

// Initialisation du bot Telegram
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Initialisation de l'écran LCD
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

const int gasSensorPin = 36; // GPIO36 pour le capteur de gaz
const int ledgreen = 3;
const int ledorange = 22; // GPIO2 pour LED
const int ledred = 21;
const int buzzerPin = 2; // GPIO15 pour Buzzer
const int doorPin = 4; // GPIO4 pour le contrôle de la porte (servo)

// Servo moteur
Servo doorServo;

const int maxGasValue = 4095; // Valeur maximale que le capteur peut lire

bool dangerSent = false;  // Pour éviter d'envoyer des messages répétés pour le même événement
bool veryDangerSent = false;
bool outOfDangerSent = true;  // Pour gérer l'état "OUT OF DANGER"

void setup() {
  Serial.begin(115200);
  pinMode(gasSensorPin, INPUT);
  pinMode(ledgreen, OUTPUT);
  pinMode(ledorange, OUTPUT);
  pinMode(ledred, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Initialisation du servo
  doorServo.attach(doorPin);
  doorServo.write(0); // Fermer la porte initialement

  // Initialisation de l'écran LCD
  WiFi.begin(ssid, password);
  lcd.begin(16, 2);
  lcd.setCursor(2, 0);
  lcd.print("GAS DETECTOR");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("----");
  delay(500);
  lcd.setCursor(4, 1);
  lcd.print("----");
  delay(500);
  lcd.setCursor(8, 1);
  lcd.print("----");
  delay(500);
  lcd.setCursor(12, 1);
  lcd.print("----");
  delay(500);
  lcd.clear();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  client.setInsecure(); // Ceci est nécessaire pour ESP32
}

void sendTelegramMessage(String message) {
  if (bot.sendMessage("CHAT_ID", message, "")) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Failed to send message");
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    Serial.println("Received message: " + text);

    if (text == "open_door") {
      Serial.println("Opening door...");
      doorServo.write(180); // Ouvrir la porte
      delay(1000); // Attendre 1 seconde pour permettre au servo de se déplacer
      sendTelegramMessage("The door is opened.");
    }
    if (text == "close_door") {
      Serial.println("Closing door...");
      doorServo.write(0); // Fermer la porte
      delay(1000); // Attendre 1 seconde pour permettre au servo de se déplacer
      sendTelegramMessage("The door is closed.");
    }
  }
}

void loop() {
  float gasLevel = analogRead(gasSensorPin);
  float gasPercentage = (gasLevel / maxGasValue) * 100;

  lcd.setCursor(0, 0);
  lcd.print("GAS =");
  lcd.print(gasPercentage);
  lcd.print(" %   ");

  if (gasPercentage < 25.0) {
    //noTone(buzzerPin);
    digitalWrite(ledred, LOW);
    digitalWrite(ledgreen, HIGH);
    digitalWrite(ledorange, LOW);

    lcd.setCursor(0, 1);
    lcd.print("<OUT OF DANGER.>");

    if (!outOfDangerSent) {
      sendTelegramMessage("Info: Gas level is now out of danger.");
      outOfDangerSent = true;  // Marquer le message comme envoyé
      dangerSent = false;      // Réinitialiser les autres états
      veryDangerSent = false;
    }
  } else {
    outOfDangerSent = false;  // Réinitialiser l'état "OUT OF DANGER" lorsqu'il y a du danger

    if (gasPercentage < 50) {
      tone(buzzerPin, 100, 200);
      digitalWrite(ledorange, HIGH);
      digitalWrite(ledgreen, LOW);
      digitalWrite(ledred, LOW);

      lcd.setCursor(0, 1);
      lcd.print("<    DANGER    >");

      if (!dangerSent) {
        sendTelegramMessage("Warning: Gas level is in the danger zone!");
        dangerSent = true;
        veryDangerSent = false;  // Réinitialiser l'état "veryDanger"
      }
    } else {
      tone(buzzerPin, 600, 700);
      digitalWrite(ledred, HIGH);
      digitalWrite(ledgreen, LOW);
      digitalWrite(ledorange, LOW);

      lcd.setCursor(0, 1);
      lcd.print("<VERY DANGER !!>");

      if (!veryDangerSent) {
        sendTelegramMessage("Critical: Gas level is in the very dangerous zone!");
        veryDangerSent = true;
        dangerSent = false;  // Réinitialiser l'état "danger"
      }
    }
  }

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while (numNewMessages) {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}
