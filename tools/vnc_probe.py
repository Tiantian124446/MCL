import socket
s=socket.create_connection(("127.0.0.1",5907),timeout=5)
banner=s.recv(12)
print("banner",banner)
s.sendall(b"RFB 003.008\n")
n=s.recv(1)
print("nsec",n)
if n and n[0]>0:
    types=s.recv(n[0])
    print("types",list(types))
s.close()
