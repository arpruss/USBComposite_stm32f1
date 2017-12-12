from pywinusb import hid
from time import sleep

def sample_handler(data):
    print("Raw data: {0}".format(data))
    
device = hid.HidDeviceFilter(vendor_id = 0x1EAF, product_id = 0x0024).get_devices()[0]
print(device)
device.open()
device.set_raw_data_handler(sample_handler)

while True:
    for out_report in device.find_feature_reports():
        print("Send")
        buffer=[i for i in range(33)]
        buffer[0]=0x0 # report id
        out_report.set_raw_data(buffer)
        out_report.send()
        sleep(2)
        