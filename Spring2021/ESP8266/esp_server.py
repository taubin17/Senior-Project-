#!/usr/bin/python

import socket

host = '192.168.1.68'
port = 65432

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((host, port))
    s.listen()
    conn, addr = s.accept()

    with conn:
        print("Connected by ", addr)
        while True:
            data = conn.recv(1024)
            conn.sendall(data)
