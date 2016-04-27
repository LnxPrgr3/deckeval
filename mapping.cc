#include "mapping.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>

static long page_size() {
	static long res = -1;
	if(res < 0) {
		res = sysconf(_SC_PAGESIZE);
		if(res < 0)
			throw std::runtime_error(strerror(errno));
	}
	return res;
}

mapping::mapping(void *addr, size_t length, int prot, int flags, int fd, off_t offset, size_t valid_length) {
	_addr = mmap(addr, length, prot, flags, fd, offset);
	if(_addr == MAP_FAILED)
		throw std::runtime_error(strerror(errno));
	_length = length;
	_valid_length = valid_length;
}

mapping::~mapping() {
	if(_addr != MAP_FAILED) {
		if(munmap(_addr, _length))
			throw std::runtime_error(strerror(errno));
	}
}

mapping::options &mapping::options::file(const class file &file) {
	const auto size = file.size();
	const auto page_size = ::page_size();
	_length = (size / page_size) * page_size + (size % page_size ? page_size : 0);
	_prot &= ~PROT_WRITE;
	_flags &= ~(MAP_ANONYMOUS | MAP_PRIVATE);
	_flags = (_flags & ~(MAP_ANONYMOUS | MAP_PRIVATE)) | MAP_FILE | MAP_SHARED;
	_fd = file.fd();
	_valid_length = size;
	return *this;
}
