import hikvisionbroker_extension
import time
cnt = [0, 0]
def callback(x):
    cnt[0] = cnt[0] + 1; t=time.time(); print(1/(t-cnt[1])); cnt[1]=t


camera = hikvisionbroker_extension.Camera(0)
camera.register_callback(callback)
camera.set_io_configuration()
camera.set_frame_rate(2.)
camera.start_acquisition()
time.sleep(3)
camera.stop_acquisition()
