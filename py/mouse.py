import sys
import struct
import multiprocessing
import serialProcess

def getMouseEvent(taskQ):
  buf = file.read(3);
  button = ord( buf[0] );
  bLeft = button & 0x1;
  bRight = ( button & 0x2 ) > 0;
  bMiddle = ( button & 0x4 ) > 0;
  x,y = struct.unpack( "bb", buf[1:] );
  taskQ.put("l%d;m%d;r%d;x%d;y%d;" % (bLeft,bMiddle,bRight, x, y) );
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
    
  file = open( "/dev/input/mice", "rb" );

  while( 1 ):
    getMouseEvent()
    if not resultQ.empty():
      print resultQ.get()

  file.close()
  sp.close()

if __name__ == "__main__":
    main()
