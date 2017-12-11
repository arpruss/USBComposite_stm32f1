from pywinusb import hid
from time import sleep

def sample_handler(data):
    print("Raw data: {0}".format(data))
    
device = hid.HidDeviceFilter(vendor_id = 0x1EAF, product_id = 0x0024).get_devices()[0]
print(device)
device.open()

while True:
    for out_report in device.find_feature_reports():
        print(out_report)
        buffer=[0x0]*33
        buffer[0]=0x4 # report id
        buffer[1]=0b11111111
        buffer[2]=0x01010101
        buffer[3]=0x10101010
        out_report.set_raw_data(buffer)
        out_report.send()
        sleep(5)
        