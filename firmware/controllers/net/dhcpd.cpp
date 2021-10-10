#include "pch.h"

#include "thread_controller.h"

#include "lwip/sockets.h"

char packetBuffer[1024];


struct DhcpDiscover {
	uint8_t MessageType;
	uint8_t HardwareType;
	uint8_t AddressLength;
	uint8_t Hops;

	uint32_t TransactionId;

	uint16_t SecondsElapsed;

	uint16_t BootpFlags;

	uint32_t ClientIpAddress, YourIp, NextServerIp, RelayIp;

	uint8_t ClientMac[16];



};


class DhcpdThread : public ThreadController<5000> {
public:
	DhcpdThread() : ThreadController("dhcp", NORMALPRIO) { }

	void ThreadTask() override {
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_port = htons(67);
		address.sin_addr.s_addr = INADDR_ANY;

		auto s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
		lwip_bind(s, (sockaddr*)&address, sizeof(address));

		while (true) {
			sockaddr_in clientAddr;
			socklen_t clientAddrLen = sizeof(clientAddr);
			int r = lwip_recvfrom(s, packetBuffer, 1024, MSG_DONTWAIT, (sockaddr*)&clientAddr, &clientAddrLen);



			DhcpDiscover* packet = reinterpret_cast<DhcpDiscover*>(packetBuffer);

			chThdSleepMilliseconds(100);
		}
	}
};

static DhcpdThread dhcp;

void startDhcpd() {
	dhcp.Start();
}
