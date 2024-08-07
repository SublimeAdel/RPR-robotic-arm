#include <WiFi.h>
#include <WebServer.h>
#include <AccelStepper.h>
#include <Servo.h>
#include <FS.h>
#include <SPIFFS.h>

// Wi-Fi credentials
const char* ssid = "wifi_ssid";
const char* password = "wifi_password";

// Web server on port 80 for HTTP
WebServer server(80);

// Stepper motor pins
#define PRISMATIC_STEP_PIN 4
#define PRISMATIC_DIR_PIN 5
#define ROTATIONAL_STEP_PIN 16
#define ROTATIONAL_DIR_PIN 17

// Servo motor pins
#define ARM_SERVO_PIN 18
#define GRIPPER_SERVO_PIN 19

// Initialize stepper motors
AccelStepper prismaticStepper(AccelStepper::DRIVER, PRISMATIC_STEP_PIN, PRISMATIC_DIR_PIN);
AccelStepper rotationalStepper(AccelStepper::DRIVER, ROTATIONAL_STEP_PIN, ROTATIONAL_DIR_PIN);

// Initialize servo motors
Servo armServo;
Servo gripperServo;

// Movement steps
const int DEFAULT_STEP_INCREMENT = 20; // Default steps or degrees
int stepIncrement = DEFAULT_STEP_INCREMENT;
long prismaticPosition = 0;
long rotationalPosition = 0;
int armPosition = 90; // Start at mid position
int gripperPosition = 90; // Start at mid position

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize web server
  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.serveStatic("/index.html", SPIFFS, "/index.html");
  server.begin();
  Serial.println("Web server started");

  // Initialize steppers
  prismaticStepper.setMaxSpeed(1000);
  prismaticStepper.setAcceleration(500);
  rotationalStepper.setMaxSpeed(1000);
  rotationalStepper.setAcceleration(500);

  // Attach servos
  armServo.attach(ARM_SERVO_PIN);
  gripperServo.attach(GRIPPER_SERVO_PIN);
}

void loop() {
  server.handleClient();
  prismaticStepper.run();
  rotationalStepper.run();
}

void handleRoot() {
  server.sendHeader("Location", "/index.html");
  server.send(302);
}

void handleMove() {
  if (server.hasArg("updateStepIncrement")) {
    stepIncrement = server.arg("updateStepIncrement").toInt();
    server.send(200, "text/plain", "Step increment updated");
    return;
  }

  if (server.hasArg("motor") && server.hasArg("direction") && server.hasArg("stepIncrement")) {
    String motor = server.arg("motor");
    String direction = server.arg("direction");
    int increment = server.arg("stepIncrement").toInt();

    if (motor == "prismatic") {
      if (direction == "up") {
        prismaticPosition += increment;
      } else if (direction == "down") {
        prismaticPosition -= increment;
      }
      prismaticStepper.moveTo(prismaticPosition);
    } else if (motor == "rotation") {
      if (direction == "cw") {
        rotationalPosition += increment;
      } else if (direction == "ccw") {
        rotationalPosition -= increment;
      }
      rotationalStepper.moveTo(rotationalPosition);
    } else if (motor == "arm") {
      if (direction == "cw") {
        armPosition += increment;
      } else if (direction == "ccw") {
        armPosition -= increment;
      }
      armServo.write(armPosition);
    } else if (motor == "gripper") {
      if (direction == "open") {
        gripperPosition += increment;
      } else if (direction == "close") {
        gripperPosition -= increment;
      }
      gripperServo.write(gripperPosition);
    }
  }

  server.send(200, "text/plain", "Move command received");
}