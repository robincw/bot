import sys
import struct
import multiprocessing
import serialProcess
from operator import add

def getMouseEvent(file):
  buf = file.read(3);
  button = ord( buf[0] );
  bLeft = button & 0x1;
  bRight = ( button & 0x2 ) > 0;
  bMiddle = ( button & 0x4 ) > 0;
  x,y = struct.unpack( "bb", buf[1:] );
  return [bLeft,bMiddle,bRight, x, y];
  # return stuffs

def main():
  taskQ = multiprocessing.Queue()
  resultQ = multiprocessing.Queue()
  serialPort = "/dev/ttyUSB0"
  
  if(len(sys.argv)>1):
    serialPort = sys.argv[1]

  sp = serialProcess.SerialProcess(serialPort, 9600, taskQ, resultQ)
  sp.daemon = True
  sp.start()
    
  mouseDevice = open( "/dev/input/mouse1", "rb" );

  x = 0;
  y = 0;
  while( 1 ):
    mouseEvent = getMouseEvent(mouseDevice);
    x = x + mouseEvent[3];
    y = y + mouseEvent[4];
    mouseEvent[3] = 0;
    mouseEvent[4] = 0;

    if x > 16:
      mouseEvent[3] = 1;
    if x < -16:
      mouseEvent[3] = -1;
    if mouseEvent[3] != 0:
      x = 0;

    if y > 16:
      mouseEvent[4] = 1;
    if y < -16:
      mouseEvent[4] = -1;
    if mouseEvent[4] != 0:
      y = 0;

    taskQ.put("l%d;m%d;r%d;x%d;y%d;" % tuple(mouseEvent) );
    
    if not resultQ.empty():
      print resultQ.get()

  mouseDevice.close()
  sp.close()

if __name__ == "__main__":
    main()
