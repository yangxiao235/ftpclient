import sys
import socket

if len(sys.argv) < 4:
    print("Usage: client_send_file file severip serverport\n")
    sys.exit()
    
filesend, serverIP, port = sys.argv[1], sys.argv[2], int(sys.argv[3])

# create an ipv4 (AF_INET) socket object using the tcp protocol (SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect the client
client.connect((serverIP, port))

file = open(filesend, "rb")
while True:
    contents = file.read()
    if not contents:
        break
    client.send(contents)
file.close()
client.close()