import sys
import socket
# ------------------------------------------
# Usage: client_send_dir severip serverport
# for example:
#   client_send_dir 127.0.0.1 6000
# ------------------------------------------
if len(sys.argv) != 3:
    print("Usage: client_send_dir severip serverport\n")
    sys.exit()
serverIP, port = sys.argv[1], int(sys.argv[2])

# create an ipv4 (AF_INET) socket object using the tcp protocol (SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect the client
client.connect((serverIP, port))

# receive the response data (4096 is recommended buffer size)
file = open("file_on_server_recv_client_pass_req.txt", "wb+");
while 1:
    response = client.recv(4096)
    if not response:
        break
    file.write(response)
file.close()
client.close()
