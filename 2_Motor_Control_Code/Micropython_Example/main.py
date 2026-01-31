from motor_control_example import DualMotorController

mc = DualMotorController()

while True:
    mc.run_demo(True)
    mc.run_demo(False)



# from neopixel_example import neopixel_demo

# neopixel_demo()