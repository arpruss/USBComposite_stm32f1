from pywinusb import hid
from time import sleep,time


count = 0
start = time()

def sample_handler(data):
    global count
    count += 1
    if count % 200 == 0:
        t = time()
        print(count,t,count/(t-start))
        print("Raw data: {0}".format(data))
    
device = hid.HidDeviceFilter(vendor_id = 0x1EAF).get_devices()[0]
print(device)
device.open()
device.set_raw_data_handler(sample_handler)

n = 0
while True:
    sleep(1)
