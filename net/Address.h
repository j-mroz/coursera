#ifndef ADDRESS_H_
#define ADDRESS_H_

#include <string>
#include <cstring>
#include <arpa/inet.h>

class Address {
public:
	char addr[6] = {0, 0, 0, 0, 0, 0};

	Address()                  = default;
	Address(const Address&)    = default;
	Address(const std::string &address) {
		size_t pos = address.find(":");
		uint32_t id   = (uint32_t)stoul(address.substr(0, pos));
		uint16_t port = (uint16_t)stoul(address.substr(pos + 1));
		memcpy(&addr[0], (char *)&id, sizeof(uint32_t));
		memcpy(&addr[4], (char *)&port, sizeof(uint16_t));
	}
	Address(int32_t id, int16_t port) {
		memcpy(&addr[0], (char *)&id, sizeof(uint32_t));
		memcpy(&addr[4], (char *)&port, sizeof(uint16_t));
	}
	Address& operator=(const Address&) = default;
	bool operator==(const Address &) const;

	std::string getAddress() const {
		return std::to_string(getIp()) + ":" + std::to_string(getPort());
	}

	std::string str() const {
		return std::string(inet_ntoa(in_addr{getIp()})) + ":" + std::to_string(getPort());
	}


	uint16_t getPort() const {
		uint16_t port;
		memcpy(&port, addr + 4, sizeof(uint16_t));
		return port;
	}

	uint32_t getIp() const {
		uint32_t ip;
		memcpy(&ip, addr, sizeof(uint32_t));
		return ip;
	}
};

#endif
