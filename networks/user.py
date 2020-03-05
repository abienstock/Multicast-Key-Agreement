import socket, sys, struct

def get_key(host, port):
	s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((host, port))
	data = input()
	s.sendall(bytes(data, 'utf-8'))
	data = s.recv(1024)
	return str(data)

def listen(addr, port):
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	group = socket.inet_aton(addr)
	mreq = struct.pack('4sL', group, socket.INADDR_ANY)
	s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
	#s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
	s.bind(('', port))

	while True:
		data, address = s.recvfrom(1024)
		print(data, address)
		s.sendto(b'ack', address)


if __name__ == "__main__":
	if len(sys.argv) != 5:
		exit("Incorrect number of arguments.")
	oob_host = sys.argv[1]
	oob_port = int(sys.argv[2])
	multicast_addr = sys.argv[3]
	multicast_port = int(sys.argv[4])

	key = get_key(oob_host, oob_port)
	print(key)

	listen(multicast_addr, multicast_port)