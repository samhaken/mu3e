''' Author: A. Loreti ok20686@bris.ac.uk  phys2114@ox.ac.uk, Modified by S. Haken ji20840@bristol.ac.uk
    Date: July 2023
    Abstract: read mid files into numpy arrays
'''

import sys
sys.path.append('/software/mu3e/Singularities/packages/midas/python/')
import midas.file_reader
import matplotlib.pyplot as plt


if len(sys.argv) != 2:
    print("You need to supply the midas filename as the only argument")
    exit()

# DATAFRAME INITIALIZATION
data = { 'Temp' : [] , 
         'DUT_Temp' : [] ,
         'Air_Temp' : [] ,
         'Air_Flow' : [] ,
         'Flow' : [],
         'Avg' : [] ,
         'Setpoint' : [] ,
         'Humidity' : [] ,
         'Ambient' : []} 

# FILE I/O
mfile = midas.file_reader.MidasFile(sys.argv[1])

# READ file
while mfile.read_next_event_header():
    
    header = mfile.event.header
    if header.is_midas_internal_event():
        # Skip over events that contain midas messages or ODB dumps
        continue
    
    mfile.read_this_event_body()     

    data_tmp = []
    for name, bank in mfile.event.banks.items():
        if name.startswith("TS"):
            this_data = [x for x in bank.data] 
            data_tmp.append(this_data)

    # order of banks: Temp, DUT Temp, Air Temp, Air Flow, Flow, Avg Flow, Set Point, Rel Humidity, Amb Temp
    if data_tmp != []:
        data["Temp"].append(data_tmp[0])
        data["DUT_Temp"].append(data_tmp[1])
        data["Air_Temp"].append(data_tmp[2])
        data["Air_Flow"].append(data_tmp[3])
        data["Flow"].append(data_tmp[4])
        data["Avg"].append(data_tmp[5])
        data["Setpoint"].append(data_tmp[6])
        data["Humidity"].append(data_tmp[7])
        data["Ambient"].append(data_tmp[8])

fig, axes = plt.subplots(2, 2, figsize=(12, 8))

axes[0,0].scatter(range(1, len(data["Temp"]) + 1), data["Temp"], label='Sensor Temperature', s=10)
axes[0,0].scatter(range(1, len(data["DUT_Temp"]) + 1), data["DUT_Temp"], label='DUT Temperature', s=10)
axes[0,0].scatter(range(1, len(data["Air_Temp"]) + 1), data["Air_Temp"], label='Thermal Air Temperature', s=10)
axes[0,0].scatter(range(1, len(data["Ambient"]) + 1), data["Ambient"], label='Ambient Temperature', s=10)
axes[0,0].scatter(range(1, len(data["Setpoint"]) + 1), data["Setpoint"], label='Setpoint', s=10, marker='x')
axes[0,0].set_xlabel("time (s)")
axes[0,0].set_ylabel("temperature (C)")
axes[0,0].set_ylim(10, 40)
axes[0,0].legend()
#plt.savefig('Temperatures.png')


axes[0,1].scatter(range(1, len(data["Air_Flow"]) + 1), data["Air_Flow"], label='Flow In', s=10)
axes[0,1].scatter(range(1, len(data["Flow"]) + 1), data["Flow"], label='Flow Out', s=10)
axes[0,1].scatter(range(1, len(data["Avg"]) + 1), data["Avg"], label='Avg Flow', s=10)
axes[0,1].set_xlabel("time (s)")
axes[0,1].set_ylabel("standard litres per minute (slm)")
axes[0,1].legend()
#plt.savefig('Flow.png')

axes[1,0].scatter(range(1, len(data["Humidity"]) + 1), data["Humidity"], label='Relative Humidity', s=10)
axes[1,0].set_xlabel("time (s)")
axes[1,0].set_ylabel("Relative Humidity")
axes[1,0].set_ylim(40, 60)
axes[1,0].legend()


plt.savefig("TestStand.png")




# plt.scatter(range(1,len(data["Temp"])+1), data["Temp"], s=10, c='steelblue', marker = 'v', label='temperature')
# plt.scatter(range(1,len(data["Flow"])+1), data["Flow"], s=10, c='forestgreen', marker = '^', label='flow')
# plt.scatter(range(1,len(data["Press"])+1), data["Press"], s=10, c='orangered', marker = 'o', label='pressure')
# plt.scatter(range(1,len(data["Setpoint"])+1), data["Setpoint"], s=10, c='violet', marker = 'x', label='setpoint')
# plt.legend()
# plt.xlabel("time (s)")
# #plt.xlim()
# plt.ylim(-5, 140)
# #plt.show()
# plt.savefig('run49.png')  



    # print("Overall size of event,type ID and bytes")
    # print((header.serial_number, header.event_id, header.event_data_size_bytes))
        # if isinstance(bank.data, tuple) and len(bank.data):
        #     # A tuple of ints/floats/etc (a tuple is like a fixed-length list)
        #     type_str = "tuple of %s containing %d elements" % (type(bank.data[0]).__name__, len(bank.data))
        # elif isinstance(bank.data, tuple):
        #     # A tuple of length zero
        #     type_str = "empty tuple"
        # elif isinstance(bank.data, str):
        #     # Of the original data was a list of chars, we convert to a string.
        #     type_str = "string of length %d" % len(bank.data)
        # else:
        #     # Some data types we just leave as a set of bytes.
        #     type_str = type(bank.data[0]).__name__
        
        # print("  - bank %s contains %d bytes of data. Python data type: %s" % (name, bank.size_bytes, type_str))



