#include <shared_memory>
#include <rpc>
#include <unordered_map>
#include <mutex>
#include <registry>

static std::unordered_map<uint64_t, uint8_t*> blocks;
static std::mutex lock;

bool read(std::PID client, std::SMID smid, uint64_t lba) {
	if(!std::registry::has(client, "RAMBLOCK_READ"))
		return false;

	auto link = std::sm::link(client, smid);
	size_t npages = link.s;
	if(!npages)
		return false;
	uint8_t* buffer = link.f;

	lock.acquire();
	while(npages--) {
		if(blocks.has(lba))
			memcpy(buffer, blocks[lba], PAGE_SIZE);
		else
			memset(buffer, 0, PAGE_SIZE);

		buffer += PAGE_SIZE;
		++lba;
	}
	lock.release();

	std::sm::unlink(smid);
	return true;
}

bool write(std::PID client, std::SMID smid, uint64_t lba) {
	if(!std::registry::has(client, "RAMBLOCK_WRITE"))
		return false;

	auto link = std::sm::link(client, smid);
	size_t npages = link.s;
	if(!npages)
		return false;
	uint8_t* buffer = link.f;

	lock.acquire();
	while(npages--) {
		if(!blocks.has(lba))
			blocks[lba] = (uint8_t*)std::mmap();

		memcpy(blocks[lba], buffer, PAGE_SIZE);

		buffer += PAGE_SIZE;
		++lba;
	}
	lock.release();

	std::sm::unlink(smid);
	return true;
}

extern "C" void _start() {
	std::exportProcedure((void*)read, 2);
	std::exportProcedure((void*)write, 2);
	std::enableRPC();
	std::publish("ramblock");
	std::halt();
}
