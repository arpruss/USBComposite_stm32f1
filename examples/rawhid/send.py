from pywinusb import hid
from time import sleep

SIZE=32

def sample_handler(data):
    print("Raw data: {0}".format(data))
    
device = hid.HidDeviceFilter(vendor_id = 0x1EAF, product_id = 0x0024).get_devices()[0]
print(device)
device.open()
device.set_raw_data_handler(sample_handler)

while True:
    for out_report in device.find_output_reports():
        print("sending")
        buffer=[i for i in range(SIZE+1)]
        buffer[0]=0x0 # report id
        out_report.set_raw_data(buffer)
        out_report.send()
        #sleep(0.001)
        