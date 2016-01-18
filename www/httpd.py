#!/usr/bin/python27
 
import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.gen
from tornado.options import define, options
 
import time
import multiprocessing
import serialProcess
 
define("port", default=12308, help="Run www server on the given port", type=int)
define("serialPort", default="/dev/ttyACM0", help="Send commands to Arduino on this serial port")
define("serialBaud", default=9600, help="Set serial speed to this baud", type=int)
define("docroot", default="/home/pi/git/bot/www/html/", help="www document root")

clients = []

 
class IndexHandler(tornado.web.RequestHandler):
    def get(self):
        self.set_header("Cache-control", "no-cache")
        self.set_header("Etag", "")
        self.render('index.html')
        
class StaticFileNoHash(tornado.web.StaticFileHandler):

    def compute_etag(self):
        return None

    @classmethod
    def get_content_version(cls, abspath):
        return 1

    @classmethod
    def _get_cached_version(cls, abs_path):
        return None

class WebSocketHandler(tornado.websocket.WebSocketHandler):
    def open(self):
        print 'new connection'
        clients.append(self)
        self.write_message("{\"msg\":\"connected\"}")
 
    def on_message(self, message):
        print 'tornado received from client: %s' % message
        #self.write_message('tornado received from client: %s' % message)
        q = self.application.settings.get('queue')
        q.put(message)
 
    def on_close(self):
        q = self.application.settings.get('queue')
	q.put("s")
	print 'connection closed'
        clients.remove(self)
 
################################ MAIN ################################
 
def main():
 
    tornado.options.parse_command_line()

    taskQ = multiprocessing.Queue()
    resultQ = multiprocessing.Queue()
 
    sp = serialProcess.SerialProcess(options.serialPort, options.serialBaud, taskQ, resultQ)
    sp.daemon = True
    sp.start()
 
    app = tornado.web.Application(
        handlers=[
            (r"/ws", WebSocketHandler),
            (r'/(.*)', StaticFileNoHash, {'path': options.docroot})
        ], queue=taskQ
    )
    httpServer = tornado.httpserver.HTTPServer(app)
    httpServer.listen(options.port)
    print "Listening on port:", options.port
    print "Arduino on serial port:", options.serialPort
    print "Serial port speed:", options.serialBaud
    #tornado.ioloop.IOLoop.instance().start()
 
    def checkResults():
        if not resultQ.empty():
            result = resultQ.get()
            print "tornado received from arduino: " + result
            for c in clients:
                c.write_message(result)
 
    mainLoop = tornado.ioloop.IOLoop.instance()
    scheduler = tornado.ioloop.PeriodicCallback(checkResults, 10, io_loop = mainLoop)
    scheduler.start()
    mainLoop.start()
 
if __name__ == "__main__":
    main()

