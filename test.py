import hikvisionbroker_extension
import time
cnt = [0]
def callback(x):
    print(cnt[0], len(x[0]), len(x[1])); cnt[0] = cnt[0] + 1


camera = hikvisionbroker_extension.Camera(0)
camera.register_callback(callback)
camera.set_frame_rate(50.)
camera.start_acquisition()
time.sleep(3)
camera.stop_acquisition()
