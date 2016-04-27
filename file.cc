#include "file.h"
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>

file::file(const char *path, int flags, mode_t mode) {
	_fd = open(path, flags, mode);
	if(_fd < 0)
		throw std::runtime_error(strerror(errno));
}

file::~file() {
	if(_fd >= 0) {
		if(close(_fd))
			throw std::runtime_error(strerror(errno));
	}
}

off_t file::size() const {
	struct stat st;
	if(fstat(_fd, &st))
		throw std::runtime_error(strerror(errno));
	return st.st_size;
}
