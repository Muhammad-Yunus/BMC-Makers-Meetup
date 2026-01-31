from machine import Pin, PWM
import time

class DualMotorController:
    def __init__(self):
        # ---------------- Pins ----------------
        self.MOTOR_A_DIR = 7
        self.MOTOR_A_PWM = 6
        self.MOTOR_B_DIR = 5
        self.MOTOR_B_PWM = 4

        self.ENCODER_A_PIN = 2
        self.ENCODER_B_PIN = 1

        # ---------------- Config ----------------
        self.PWM_FREQ = 5000
        self.MAX_PWM = 255
        self.ENCODER_PULSES_PER_REV = 20
        self.ACCEL_STEP = 5
        self.ACCEL_DELAY = 200  # ms

        # ---------------- State ----------------
        self.encoderA_count = 0
        self.encoderB_count = 0
        self.last_speed_check = time.ticks_ms()
        self.speed_A_RPM = 0.0
        self.speed_B_RPM = 0.0

        # ---------------- Hardware init ----------------
        self.motorA_dir = Pin(self.MOTOR_A_DIR, Pin.OUT)
        self.motorB_dir = Pin(self.MOTOR_B_DIR, Pin.OUT)

        self.motorA_pwm = PWM(Pin(self.MOTOR_A_PWM))
        self.motorB_pwm = PWM(Pin(self.MOTOR_B_PWM))
        self.motorA_pwm.freq(self.PWM_FREQ)
        self.motorB_pwm.freq(self.PWM_FREQ)

        # Initial direction: forward
        self.motorA_dir.on()
        self.motorB_dir.on()

        # Encoder pins
        self.encoderA = Pin(self.ENCODER_A_PIN, Pin.IN, Pin.PULL_UP)
        self.encoderB = Pin(self.ENCODER_B_PIN, Pin.IN, Pin.PULL_UP)

        # Attach interrupts
        self.encoderA.irq(trigger=Pin.IRQ_RISING, handler=self._encA_isr)
        self.encoderB.irq(trigger=Pin.IRQ_RISING, handler=self._encB_isr)

        print("DualMotorController initialized")

    # ---------------- Encoder ISR ----------------
    def _encA_isr(self, pin):
        self.encoderA_count += 1

    def _encB_isr(self, pin):
        self.encoderB_count += 1

    # ---------------- Motor control ----------------
    def set_motor_speed(self, motor, pwm, forward=True):
        pwm = max(0, min(self.MAX_PWM, pwm))
        if forward:
            pwm = 255 - pwm

        duty = int(pwm * 1023 / 255)  # ESP32 PWM is 10-bit

        if motor == 0:
            self.motorA_dir.value(1 if forward else 0)
            self.motorA_pwm.duty(duty)
        elif motor == 1:
            self.motorB_dir.value(1 if forward else 0)
            self.motorB_pwm.duty(duty)

    # ---------------- Speed calculation ----------------
    def update_speed(self):
        now = time.ticks_ms()
        if time.ticks_diff(now, self.last_speed_check) >= self.ACCEL_DELAY:
            pA = self.encoderA_count
            pB = self.encoderB_count
            self.encoderA_count = 0
            self.encoderB_count = 0

            interval_sec = self.ACCEL_DELAY / 1000.0
            self.speed_A_RPM = (pA / self.ENCODER_PULSES_PER_REV) * 60 / interval_sec
            self.speed_B_RPM = (pB / self.ENCODER_PULSES_PER_REV) * 60 / interval_sec
            self.last_speed_check = now

    # ---------------- Print speed ----------------
    def print_speed(self, pwm):
        print(
            "Motor A: {:.1f} RPM | Motor B: {:.1f} RPM | PWM: {}".format(
                self.speed_A_RPM, self.speed_B_RPM, pwm
            )
        )

    # ---------------- Gradual acceleration / deceleration ----------------
    def run_demo(self, forward=True):
        direction = "Forward" if forward else "Reverse"
        print("\nStarting gradual acceleration ({})...\n".format(direction))

        # Accelerate
        for pwm in range(0, self.MAX_PWM + 1, self.ACCEL_STEP):
            self.set_motor_speed(0, pwm, forward)
            self.set_motor_speed(1, pwm, forward)
            time.sleep_ms(self.ACCEL_DELAY)
            self.update_speed()
            self.print_speed(pwm)

        print("\nReached maximum speed!")
        time.sleep(2)

        # Decelerate
        print("\nStarting gradual deceleration...\n")
        for pwm in range(self.MAX_PWM, -1, -self.ACCEL_STEP):
            self.set_motor_speed(0, pwm, forward)
            self.set_motor_speed(1, pwm, forward)
            time.sleep_ms(self.ACCEL_DELAY)
            self.update_speed()
            self.print_speed(pwm)

        print("\nMotors stopped!")
        time.sleep(2)
