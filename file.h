#ifndef DECKEVAL_FILE_H
#define DECKEVAL_FILE_H
#include <sys/stat.h>
#include <fcntl.h>

class file {
public:
	file(const char *path, int flags, mode_t mode);
	file(const file &) = delete;
	file(file &&x) {
		_fd = x._fd;
		x._fd = -1;
	}
	~file();
	class options {
	public:
		options(const char *path) {
			_path = path;
			_flags = O_RDONLY;
			_mode = 0777;
		}
		options &create(bool error_if_exists = false) {
			_flags |= O_CREAT;
			if(error_if_exists)
				_flags |= O_EXCL;
			return *this;
		}
		options &write() {
			_flags |= O_WRONLY;
			return *this;
		}
		file open() {
			return file(_path, _flags, _mode);
		}
	private:
		const char *_path;
		int _flags;
		mode_t _mode;
	};
	int fd() const { return _fd; }
	off_t size() const;
private:
	int _fd;
};


#endif
