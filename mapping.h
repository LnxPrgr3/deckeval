#ifndef DECKEVAL_MAPPING_H
#define DECKEVAL_MAPPING_H
#include "file.h"
#include <sys/mman.h>
#include <unistd.h>
#include <utility>

class mapping {
public:
	mapping(const mapping &) = delete;
	mapping(mapping &&x) {
		*this = std::move(x);
	}
	mapping() {
		_addr = MAP_FAILED;
		_length = -1;
		_valid_length = -1;
	}
	mapping(void *addr, size_t length, int prot, int flags, int fd, off_t offset, size_t valid_length);
	~mapping();
	class options {
	public:
		options() {
			_addr = NULL;
			_length = 0;
			_prot = PROT_READ | PROT_WRITE;
			_flags = MAP_ANONYMOUS | MAP_PRIVATE;
			_fd = -1;
			_offset = 0;
		}
		options &addr(void *addr, bool force = true) {
			_addr = addr;
			_flags = (_flags & ~MAP_FIXED) | (force ? MAP_FIXED : 0);
			return *this;
		}
		options &length(size_t length) {
			_length = length;
			_valid_length = length;
			return *this;
		}
		options &file(const file &file);
		options &write(bool write_to_backing = true) {
			_prot |= PROT_WRITE;
			if(write_to_backing)
				_flags = (_flags & ~MAP_PRIVATE) | MAP_SHARED;
			else
				_flags = (_flags & ~MAP_SHARED) | MAP_PRIVATE;
			return *this;
		}
		mapping map() {
			return mapping(_addr, _length, _prot, _flags, _fd, _offset, _valid_length);
		}
	private:
		void *_addr;
		size_t _length;
		int _prot;
		int _flags;
		int _fd;
		off_t _offset;
		size_t _valid_length;
	};
	const void *data() const { return _addr; }
	void *data() { return _addr; }
	size_t size() const { return _valid_length; }
	size_t &size() { return _valid_length; }
	void truncate(size_t size);
	static long page_size();
	mapping &operator=(mapping &&x) {
		this->~mapping();
		_addr = x._addr;
		_length = x._length;
		_valid_length = x._valid_length;
		x._addr = MAP_FAILED;
		x._length = -1;
		x._valid_length = -1;
		return *this;
	}
private:
	void *_addr;
	size_t _length;
	size_t _valid_length;
};

#endif
