#!/usr/bin/python

import socket
import sys
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_addr = ('',8080)
sock.bind(server_addr)
while 1:
  data, addr = sock.recvfrom(4096)
  print >> sys.stderr, data
