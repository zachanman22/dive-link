import threading
import queue
import reedsolo as rs
import time

class ReedSolomonThread(threading.Thread):
    def __init__(self, ecc_bytes, input_queue=None, output_queue=None):
        super().__init__()
        self.rs = rs.RSCodec(ecc_bytes)
        self.ecc_bytes = ecc_bytes
        self.should_stop = False
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.RUNNING = 0
        self.INITIALIZED = 1
        self.CRASHED = 2
        self.status = self.INITIALIZED
        print("Reed Solomon initialized with ", ecc_bytes, " error correction bytes")

    def process_next_input(self):
        # check if there is data in the "to transmit queue" to send to the serial port
        if not self.input_queue.empty():
            data = self.input_queue.get()
            print(data)
            data = [data[8*i:8*(i+1)] for i in range(int(len(data)/8))]
            data = [int(i, 2) for i in data]
            for i in range(self.ecc_bytes):
                data[-1-i] = data[-2-i]
            data[-2-self.ecc_bytes] = 0
            try:
                output = self.rs.decode(bytearray(data))[0]
                output = output.decode("utf-8")
                self.output_queue.put(output)
            except:
                self.output_queue.put("Message Decode Failed, Too Much Noise")

    def run(self):
        self.status = self.RUNNING
        while not self.should_stop:
            try:
                if self.status == self.RUNNING:
                    try:
                        # check if there is data in the main queue to send to the serial port
                        self.process_next_input()
                    except:
                        self.status = self.CRASHED
                        print("RS has crashed")
            except:
                print("Unknown Reed Solomon error, retrying in 2 secs")
                time.sleep(2)

    def stop(self):
        self.should_stop = True
        self.join()

if __name__ == "__main__":
    external_serial_thread_port = "COM11"

    # create a queue to hold the serial data
    input_queue = queue.Queue()
    output_queue = queue.Queue()
    # create and start the serial reader thread
    # messageEndToken=b'stop\n'
    external_serial_thread = ReedSolomonThread(
        ecc_bytes = 8, input_queue=input_queue, output_queue=output_queue)
    external_serial_thread.start()

    # do other stuff while serial data is being read and written on the separate thread

    # write data to the serial port by adding it to the main queue
    # main_data_queue.put(b"Hello world")
    inp = 0
    while inp != "e":
        inp = input("Press enter to send a message")
        if "e" != inp:
            input_queue.put("0100011101000111010001110100011101000111010001110100011101000111010001110100011101000111010001110100011101000111010001110100011101000111010001110100011101000111010001110100011101000111011100110011010101100100101111011011101010001010111110000001000000000000")
        while not output_queue.empty():
            data = output_queue.get()
            print(data)
    external_serial_thread.stop()

    # # read serial data from the queue
    # while True:
    #     try:
    #         input()
    #     except KeyboardInterrupt:
    #         mcu_serial_thread.stop()
    #         external_serial_thread.stop()
    #         break

    
