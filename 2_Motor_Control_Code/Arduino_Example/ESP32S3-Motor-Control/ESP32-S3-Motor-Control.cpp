#include <Arduino.h>

// Motor A pins
#define MOTOR_A_PIN1 7  // Direction
#define MOTOR_A_PIN2 6  // PWM (speed)

// Motor B pins
#define MOTOR_B_PIN1 5  // Direction
#define MOTOR_B_PIN2 4  // PWM (speed)

// Encoder pins
#define ENCODER_A_PIN 2
#define ENCODER_B_PIN 1

// PWM Configuration
#define PWM_FREQ 5000       // 5kHz PWM frequency
#define PWM_RESOLUTION 8    // 8-bit resolution (0-255)
#define MOTOR_A_CHANNEL 0
#define MOTOR_B_CHANNEL 1

// Encoder variables
volatile long encoderA_count = 0;
volatile long encoderB_count = 0;

// Encoder configuration (adjust based on your encoder)
#define ENCODER_PULSES_PER_REV 20  // Pulses per revolution

// Acceleration control
#define MAX_PWM 255
#define ACCELERATION_STEP 5  // Increase PWM by 5 every iteration
#define ACCELERATION_DELAY 200  // Delay between acceleration steps in ms

// Speed control variables
unsigned long lastSpeedCheck = 0;
const unsigned long SPEED_CHECK_INTERVAL = ACCELERATION_DELAY; // Check speed every 1 second

float speed_A_RPM = 0;
float speed_B_RPM = 0;



// ===== Interrupt Handlers for Encoders =====
void IRAM_ATTR encoderA_ISR() {
  encoderA_count++;
}

void IRAM_ATTR encoderB_ISR() {
  encoderB_count++;
}

// ===== Initialize PWM Channels =====
void initPWM() {
  // Setup PWM for Motor A (Channel 0)
  ledcSetup(MOTOR_A_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_A_PIN2, MOTOR_A_CHANNEL);
  
  // Setup PWM for Motor B (Channel 1)
  ledcSetup(MOTOR_B_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_B_PIN2, MOTOR_B_CHANNEL);
}

// ===== Initialize Encoders =====
void initEncoders() {
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderB_ISR, RISING);
}

// ===== Initialize Motor Direction Pins =====
void initMotors() {
  pinMode(MOTOR_A_PIN1, OUTPUT);
  pinMode(MOTOR_B_PIN1, OUTPUT);
  
  // Set initial direction (forward)
  digitalWrite(MOTOR_A_PIN1, HIGH);
  digitalWrite(MOTOR_B_PIN1, HIGH);
}

// ===== Set Motor Speed and Direction =====
void setMotorSpeed(int motorChannel, int pwmValue, bool forward) {
  pwmValue = forward ? 255-pwmValue : pwmValue;
  if (motorChannel == 0) {
    digitalWrite(MOTOR_A_PIN1, forward ? HIGH : LOW);
    ledcWrite(MOTOR_A_CHANNEL, constrain(pwmValue, 0, MAX_PWM));
  } else if (motorChannel == 1) {
    digitalWrite(MOTOR_B_PIN1, forward ? HIGH : LOW);
    ledcWrite(MOTOR_B_CHANNEL, constrain(pwmValue, 0, MAX_PWM));
  }
}

// ===== Calculate Speed (RPM) =====
void updateSpeed() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSpeedCheck >= SPEED_CHECK_INTERVAL) {
    // Disable interrupts temporarily to read counts safely
    noInterrupts();
    long pulses_A = encoderA_count;
    long pulses_B = encoderB_count;
    encoderA_count = 0;
    encoderB_count = 0;
    interrupts();
    
    // Calculate RPM
    // RPM = (pulses / ENCODER_PULSES_PER_REV) * 60 / (interval_in_seconds)
    float interval_seconds = SPEED_CHECK_INTERVAL / 1000.0;
    speed_A_RPM = (pulses_A / (float)ENCODER_PULSES_PER_REV) * 60.0 / interval_seconds;
    speed_B_RPM = (pulses_B / (float)ENCODER_PULSES_PER_REV) * 60.0 / interval_seconds;
    
    lastSpeedCheck = currentTime;
  }
}

// ===== Print Speed Data =====
void printSpeedData(int pwmValue) {

  Serial.print("Motor A: ");
  Serial.print(speed_A_RPM, 1);
  Serial.print(" RPM | Motor B: ");
  Serial.print(speed_B_RPM, 1);
  Serial.print(" RPM | PWM: ");
  Serial.println(pwmValue);
  
}

// ===== Gradually Accelerate Motors =====
void gradualAcceleration(bool forward = true) {
  Serial.println("Starting gradual acceleration...\n");
  
  for (int pwm = 0; pwm <= MAX_PWM; pwm += ACCELERATION_STEP) {
    setMotorSpeed(0, pwm, forward);  // Motor A forward
    setMotorSpeed(1, pwm, forward);  // Motor B forward
    
    // Measure and print speed during acceleration
    delay(ACCELERATION_DELAY);
    updateSpeed();
    printSpeedData(pwm);
  }
  
  Serial.println("\nReached maximum speed!");
  delay(2000); // Hold at max speed for 2 seconds
  
  // Gradually decelerate
  Serial.println("\nStarting gradual deceleration...\n");
  for (int pwm = MAX_PWM; pwm >= 0; pwm -= ACCELERATION_STEP) {
    setMotorSpeed(0, pwm, forward);
    setMotorSpeed(1, pwm, forward);
    
    delay(ACCELERATION_DELAY);
    updateSpeed();
    printSpeedData(pwm);
  }
  
  Serial.println("\nMotors stopped!");
  Serial.println("\nReached minimum speed!");
  delay(2000); // Hold at max speed for 2 seconds
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n=== ESP32S3 Dual Motor Control with Encoders ===\n");
  
  initPWM();
  initEncoders();
  initMotors();
  
  Serial.println("Initialization complete!");
}

// ===== Main Loop =====
void loop() {
  gradualAcceleration(true);  // Forward
  gradualAcceleration(false); // Reverse
  
}

