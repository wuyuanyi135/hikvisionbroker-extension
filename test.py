import hikvisionbroker_extension
import time

camera = hikvisionbroker_extension.Camera(0)
camera.register_callback(lambda x: print(len(x[0]), len(x[1])))
camera.set_frame_rate(1.)
camera.start_acquisition()
time.sleep(3)
camera.stop_acquisition()
