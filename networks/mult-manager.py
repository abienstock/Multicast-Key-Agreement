import socket, sys, struct

if __name__ == "__main__":
	if len(sys.argv) != 3:
		exit("Incorrect number of arguments.")
	multicast_addr = sys.argv[1]
	port = int(sys.argv[2])
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	ttl = struct.pack('b', 1)
	s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
	s.sendto(b'test', (multicast_addr, port))
	data, address = s.recvfrom(1024)
	print(data, address)
	s.close()