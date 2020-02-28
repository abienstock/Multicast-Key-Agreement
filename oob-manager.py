import socket, select, sys, struct

if __name__ == "__main__":
	if len(sys.argv) != 5:
		exit("Incorrect number of arguments.")
	oob_host = sys.argv[1]
	oob_port = int(sys.argv[2])
	multicast_addr = sys.argv[3]
	multicast_port = int(sys.argv[4])


	oob_socket=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	oob_socket.setblocking(0)
	oob_socket.bind((oob_host, oob_port))
	oob_socket.listen(5)


	multicast_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	ttl = struct.pack('b', 1)
	multicast_socket.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)


	inputs = [oob_socket, sys.stdin.fileno()]
	outputs = []
	message_queues = {}
	while inputs:
		readable, writeable, exceptional = select.select(inputs, outputs, inputs)
		print("readable", readable, "writeable", writeable)
		for c in readable:
			if c is oob_socket:
				conn, addr = c.accept()
				print(addr)
				conn.setblocking(0)
				inputs.append(conn)
				message_queues[conn] = []
			else:
				if c == 0:
					cmd = bytes(input(), 'utf-8')
					message_queues[multicast_socket] = [cmd]
					outputs.append(multicast_socket)
				else:
					data = c.recv(1024)
					if data:
						message_queues[c].append(data)
						if c not in outputs:
							outputs.append(c)
					else:
						#print('gone')
						if c in outputs:
							outputs.remove(c)
						inputs.remove(c)
						c.close()
						del message_queues[c]


		for c in writeable:
			if c is multicast_socket:
				try:
					next_msg = message_queues[c].pop(0)
					#next_msg = b'multicast'
				except:
					outputs.remove(c)
				else:
					c.sendto(next_msg, (multicast_addr, multicast_port))
					outputs.remove(c)
			else:
				try:
					next_msg = message_queues[c].pop(0)
					#next_msg = b'multicast'
				except:
					outputs.remove(c)
				else:
					c.send(next_msg)

		for c in exceptional:
			inputs.remove(c)
			if c in outputs:
				outputs.remove(c)
			c.close()
			del message_queues[c]

#	print('Connected by', addr)
#	while True:
#		data = conn.recv(1024)
#		if not data: break
#		conn.sendall(data)