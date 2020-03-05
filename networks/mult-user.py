import socket, sys, struct

if __name__ == "__main__":
	if len(sys.argv) != 3:
		exit("Incorrect number of arguments.")
	multicast_addr = sys.argv[1]
	port = int(sys.argv[2])
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	group = socket.inet_aton(multicast_addr)
	mreq = struct.pack('4sL', group, socket.INADDR_ANY)
	s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
	#s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
	s.bind(('', port))

	while True:
		data, address = s.recvfrom(1024)
		print(data, address)
		s.sendto(b'ack', address)