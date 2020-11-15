#include "engine_test_helper.h"
#include "tunerstudio_io.h"

extern int sr5TestWriteDataIndex;
extern uint8_t st5TestBuffer[16000];


#define CODE 2
#define PAYLOAD "123"
#define SIZE strlen(PAYLOAD)

static void assertCrcPacket() {
	ASSERT_EQ(sr5TestWriteDataIndex, SIZE + 7);

	// todo: proper uint16 comparison
	ASSERT_EQ(st5TestBuffer[0], 0);
	ASSERT_EQ(st5TestBuffer[1], SIZE + 1);

	ASSERT_EQ(st5TestBuffer[2], CODE);

	ASSERT_EQ(memcmp(&st5TestBuffer[3], PAYLOAD, SIZE), 0);


	// todo: proper uint32 comparison
	ASSERT_EQ(st5TestBuffer[6], 252);
	ASSERT_EQ(st5TestBuffer[7], 68);
	ASSERT_EQ(st5TestBuffer[8], 173);
	ASSERT_EQ(st5TestBuffer[9], 87);
}


TEST(binary, testWriteCrc) {

	sr5TestWriteDataIndex = 0;
	sr5WriteCrcPacket(nullptr, CODE, (const uint8_t * )PAYLOAD, SIZE);
	assertCrcPacket();

	sr5TestWriteDataIndex = 0;
	sr5WriteCrcPacketLarge(nullptr, CODE, (const uint8_t * )PAYLOAD, SIZE);
	assertCrcPacket();

	sr5TestWriteDataIndex = 0;
//	sr5WriteCrcPacketSmall(nullptr, CODE, (const uint8_t * )PAYLOAD, SIZE);
//	assertCrcPacket();

}
