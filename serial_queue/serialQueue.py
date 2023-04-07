import threading
import serial
import time


class SerialCommsThread(threading.Thread):
    def __init__(self, port, messageEndToken=b'\n', add_new_line_to_transmit=False, baudrate=115200, received_queue=None, to_transmit_queue=None):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.serial_port = serial.Serial(port, baudrate)
        self.serial_buffer = b''
        self.add_new_line_to_transmit = add_new_line_to_transmit
        self.should_stop = False
        self.messageEndToken = messageEndToken
        self.received_queue = received_queue
        self.to_transmit_queue = to_transmit_queue
        self.RUNNING = 0
        self.INITIALIZED = 1
        self.CRASHED = 2
        self.status = self.INITIALIZED
        print("Connected to serial port ", port)

    def handle_transmit_queue(self):
        # check if there is data in the "to transmit queue" to send to the serial port
        if not self.to_transmit_queue.empty():
            data = self.to_transmit_queue.get()
            if not isinstance(data, (bytes, bytearray)):
                data = data.encode()
            print("Data to transmit on port ", self.port, " : ", data)
            if self.add_new_line_to_transmit:
                data = data + b'\n'
                print("Data to transmit on port with added ",
                      self.port, " : ", data)
            self.serial_port.write(data)

    def handle_receive_serial_port(self):
        # check if there is data in the serial port buffer to read
        if self.serial_port.in_waiting > 0:
            data = self.serial_port.read(
                self.serial_port.in_waiting)
            self.serial_buffer = self.serial_buffer + data
            while self.messageEndToken in self.serial_buffer:  # split data line by line and store it in var
                messagesReceived, self.serial_buffer = self.serial_buffer.split(
                    self.messageEndToken, 1)
                print("Messages received on ",
                      self.port, ": ", messagesReceived)
                self.received_queue.put(messagesReceived)

    def restart_serial_port_on_failure(self):
        # close port if it needs to be closed
        try:
            self.serial_port.close()
            time.sleep(0.5)
        except:
            pass
        # restart serial port
        try:
            self.serial_port = serial.Serial(
                self.port, self.baudrate)
            time.sleep(0.5)
            self.serInBuffer = b''
            self.status = self.RUNNING
            print("Serial ", self.port, " running")
        except:
            pass

    def run(self):
        self.status = self.RUNNING
        while not self.should_stop:
            try:
                if self.status == self.RUNNING:
                    try:
                        # check if there is data in the main queue to send to the serial port
                        self.handle_transmit_queue()

                        # check if there is data in the serial port buffer to read
                        self.handle_receive_serial_port()
                    except:
                        self.status = self.CRASHED
                        print("Serial ", self.port, " crashed")
                else:
                    # restart port on crash
                    self.restart_serial_port_on_failure()
            except:
                print("Unknown serial error, retrying in 2 secs")
                time.sleep(2)

    def stop(self):
        self.should_stop = True
        self.join()