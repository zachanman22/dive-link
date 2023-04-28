from SlidingThread import SerialCommsThread, SlideWinThread
from ReedSolomonThread import ReedSolomonThread
# from SlidingThreadwRS import SlideWinThread
import queue
import time

external_serial_thread_port = "COM5"

# create a queue to hold the serial data
from_external_data_queue = queue.Queue()
to_RS_queue = queue.Queue()
final_message_queue = queue.Queue()
# create and start the serial reader thread
# messageEndToken=b'stop\n'
external_serial_thread = SerialCommsThread(
    external_serial_thread_port, add_new_line_to_transmit=False, baudrate=115200, received_queue=from_external_data_queue)
external_serial_thread.start()

window_thread = SlideWinThread(
    received_queue=from_external_data_queue, to_transmit_queue = to_RS_queue, fs=400000, bitrate=800, messageSize=64)
window_thread.start()

# window_thread = SlideWinThread(
#     received_queue=from_external_data_queue, to_transmit_queue = to_RS_queue, fs=400000, bitrate=400, messageSize=64, eccBytes=4)
# window_thread.start()

# rs_thread = ReedSolomonThread(
#         ecc_bytes = 4, input_queue=to_RS_queue, output_queue=final_message_queue)
# rs_thread.start()

#Make a new thread to handle goertzel algorithm


# do other stuff while serial data is being read and written on the separate thread

# write data to the serial port by adding it to the main queue
# main_data_queue.put(b"Hello world")
# # read serial data from the queue

while True:
    try:
        data = to_RS_queue.get()
        data = [data[8*i:8*(i+1)] for i in range(int(len(data)/8))]
        data = [int(i,2) for i in data]
        data = bytearray(data).decode("utf-8")
        print(data)
    except:
        print("Too Noisy")