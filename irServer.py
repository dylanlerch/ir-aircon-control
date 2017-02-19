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

        # ON call - turns the AC on with the most recent temperature
        elif self.path == "/on":
            # If bit 5 is high is a 'power' command. If the next four bits are 
            # high, it's an 'on' command
            serialport.write(b'\x1F')
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()
            self.wfile.write("Done!")

        # OFF call - turns the AC on with the most recent temperature
        elif self.path == "/off":
            # If bit 5 is high is a 'power' command. If the next four bits are 
            # low, it's an 'off' command
            serialport.write(b'\x10')
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()
            self.wfile.write("Done!")

        # Only other possibility is a temperature command, attempt to parse as
        # an integer.
        else:
            try:
                splitpath = self.path.split('/')
                # If the route is a number between 17 and 30, and there are no
                # other elements in the route
                temp = int(splitpath[1])

                if len(splitpath) == 2 and temp in range(17, 31):
                    # We have a valid temperature, send this to the arduino.
                    # Keep first 4 bits as low, to signal that this is a
                    # temperature command and now a power command
                    instruction = (temp - 17)
                    serialport.write(chr(instruction))

                    self.send_response(200)
                    self.send_header("Content-type", "text/plain")
                    self.end_headers()
                    self.wfile.write("Done!")
                else:
                    self.send_error(404)
            except:
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
