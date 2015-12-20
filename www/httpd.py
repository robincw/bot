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
 
define("port", default=12308, help="run on the given port", type=int)
 
clients = []

 
class IndexHandler(tornado.web.RequestHandler):
    def get(self):
        self.set_header("Cache-control", "no-cache")
        self.set_header("Etag", "")
        self.render('radial.html')
        
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
 
    taskQ = multiprocessing.Queue()
    resultQ = multiprocessing.Queue()
 
    sp = serialProcess.SerialProcess(taskQ, resultQ)
    sp.daemon = True
    sp.start()
 
    tornado.options.parse_command_line()
    app = tornado.web.Application(
        handlers=[
            (r"/ws", WebSocketHandler),
            (r'/(.*)', StaticFileNoHash, {'path': '/home/pi/www/'})
        ], queue=taskQ
    )
    httpServer = tornado.httpserver.HTTPServer(app)
    httpServer.listen(options.port)
    print "Listening on port:", options.port
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

