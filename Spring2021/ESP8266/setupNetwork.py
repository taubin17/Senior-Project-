#!/usr/bin/python
# Script will simply get a network ID and a password and connect to it. Will return error if unable to connect


import network

def connect(network_name, password):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)

    # If we aren't connected to a network, try to connect
    if not wlan.isconnected():
        print("Connecting to network: ", network_name)
        wlan.connect(network_name, password)
        
        # Wait for it to connect
        while not wlan.isconnected():
            pass
        print("Connected to network: ", network_name)

    else:
        print("Already connected to a network, would you like to join this network?")

def main():
    net_name = input("Please enter the network ID: ")
    password = input("Please enter a password: ")

    connect(net_name, password)

    return
