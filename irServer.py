#!/usr/bin/python
import serial
import json
from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer

PORT_NUMBER = 8080

#This class will handles any incoming request from
#the browser 
class handler(BaseHTTPRequestHandler):
    # All requests are just simple get requests - no request bodies, just 
    # calls to specific URLs
    def do_GET(self):
        splitpath = self.path.split("/")

        # Status call - returns the current status of the AC that the arduino
        # knows about. If changed have been made with the remote, the arduino
        # won't know about these.
        if self.path == "/status":
            serialport.write(b'\xFF') # Ask the arduino for the current status
            response = serialport.read() # Read the status response

            # Single byte response - first 4 bits are the temperature (from 0 -
            # 13, representing temperatures from 17 to 30), second 4 bits are on
            # and off (0 is on, 7 is off). 
            temp = ((bytearray(response)[0]) >> 4) + 17
            power = not bool((bytearray(response)[0]) & 7)

            # Convert to JSON to be sent back to the requester - this can be
            # extended in the future if there is more information that can be
            # returned.
            js = json.dumps({'temp': temp, 'on': power})
            
            self.send_response(200)
            self.send_header('Content-type','text/json')
            self.end_headers()
            self.wfile.write(js)

        # Set call - sets the AC to on or off, and a specific temperature. URL
        # is /[on/off]/[temp]. E.g. /on/24. For the sake of having a single
        # route structure for this call, off calls still must be given abs
        # temperature
        elif len(splitpath) == 3:
            try:
                # First element in the route must be 'on' or 'off'. Second
                # must be a number betwee 17 and 30 inclusive. For all requests
                # that don't meet these requirements, respond with a 404.
                if splitpath[1] in ['on', 'off'] and int(splitpath[2]) in range(17, 31):
                    # We have a valid temperature and power command, send this
                    # to the arduino.
                    on = 0
                    temp = int(splitpath[2])

                    if splitpath[1] == 'off':
                        on = 7
                    
                    instruction = ((temp - 17) << 4) + on
                    serialport.write(chr(instruction))

                    self.send_response(200)
                else:
                    self.send_error(404)
            except:
                self.send_error(404)

        else:
            self.send_error(404)


        return


try:
    server = HTTPServer(('', PORT_NUMBER), handler)
    serialport = serial.Serial('/dev/cu.usbmodem1421', 9600)

    print 'Started httpserver on port' , PORT_NUMBER

    server.serve_forever()

except KeyboardInterrupt:
	print '^C received, shutting down the web server'
	server.socket.close()
