#include <shared_memory>
#include <rpc>
#include <unordered_map>
#include <mutex>

bool connect(std::PID client, std::SMID smid) {
	return std::sm::connect(client, smid);
}

static std::unordered_map<uint64_t, uint8_t*> blocks;
static std::mutex lock;

bool read(std::PID client, uint64_t lba) {
	uint8_t* buffer = std::sm::get(client);
	if(!buffer)
		return false;

	lock.acquire();
	if(!blocks.has(lba)) {
		lock.release();
		memset(buffer, 0, PAGE_SIZE);
	} else {
		memcpy(buffer, blocks[lba], PAGE_SIZE);
		lock.release();
	}
	return true;
}

bool write(std::PID client, uint64_t lba) {
	uint8_t* buffer = std::sm::get(client);
	if(!buffer)
		return false;

	lock.acquire();
	if(!blocks.has(lba))
		blocks[lba] = (uint8_t*)std::mmap();

	memcpy(blocks[lba], buffer, PAGE_SIZE);
	lock.release();
	return true;
}

extern "C" void _start() {
	std::exportProcedure((void*)connect, 1);
	std::exportProcedure((void*)read, 1);
	std::exportProcedure((void*)write, 1);
	std::enableRPC();
	std::publish("ramblock");
	std::halt();
}
