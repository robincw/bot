import serial
import time
import multiprocessing
 
class SerialProcess(multiprocessing.Process):
 
    def __init__(self, taskQ, resultQ):
        multiprocessing.Process.__init__(self)
        self.taskQ = taskQ
        self.resultQ = resultQ
        self.usbPort = '/dev/ttyUSB0'
        self.sp = serial.Serial(self.usbPort, 115200, timeout=1)
 
    def close(self):
        self.sp.close()
 
    def run(self):
 
    	self.sp.flushInput()
 
        while True:
            # look for incoming tornado request
            if not self.taskQ.empty():
                task = str(self.taskQ.get())
 
                # send it to the arduino
                #print "arduino will be sent: " + task
                self.sp.write(task);
                #print "arduino received from tornado: " + task
 
            # look for incoming serial data
            if (self.sp.inWaiting() > 0):
            	result = str(self.sp.readline().replace("\n", ""))
 
                # send it back to tornado
            	#print "arduino replied: " + result
		self.resultQ.put(result)
