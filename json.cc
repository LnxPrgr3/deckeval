#include "json.h"

json_allocator_heap::json_allocator_heap(size_t n) {
	const auto page_size = mapping::page_size();
	n = (n / page_size) * page_size + (n % page_size ? page_size : n);
	options = mapping::options()
		.length(n);
	data = options.map();
	pos = 0;
}

void *json_allocator_base::allocate(size_t n, size_t alignment) {
	size_t align = _heap->pos % alignment ? alignment - _heap->pos % alignment : 0;
	while(_heap->pos + align + n > _heap->data.size()) {
		_heap->data.extend(_heap->options, _heap->data.size());
	}
	_heap->pos += align;
	void *rv = reinterpret_cast<char *>(_heap->data.data())+_heap->pos;
	_heap->pos += n;
	return rv;
}

bool json_string::operator<(const json_string &x) const {
	size_t n = _len < x._len ? _len : x._len;
	auto res = memcmp(_data, x._data, n);
	return res < 0 ? true :
		   res == 0 ? _len < x._len :
		   false;
}

bool json_string::operator==(const char *x) {
	size_t n = strlen(x);
	return n == _len && memcmp(_data, x, _len) == 0;
}

bool json_string::operator!=(const char *x) {
	size_t n = strlen(x);
	return n != _len || memcmp(_data, x, _len) != 0;
}

template class json_value_imp<std::allocator<void>>;
template class json_value_imp<json_allocator<char>>;

json_document::~json_document() {
	value_type *self = dynamic_cast<value_type *>(this);
	self->~value_type();
	_type = NONE;
}

json_document::value_type json_document::convert(const json_value &x) {
	if(x.is_boolean())
		return value_type(x.as_boolean());
	else if(x.is_number())
		return value_type(x.as_number());
	else if(x.is_string())
		return value_type(x.as_string().c_str(), x.as_string().c_str()+x.as_string().size());
	else if(x.is_object())
		return value_type(json_object(), _allocator);
	else if(x.is_array())
		return value_type(json_array(), _allocator);
	return value_type();
}

void json_document::shrink_to_fit() {
	const auto page_size = mapping::page_size();
	size_t size = (_heap.pos / page_size) * page_size + (_heap.pos % page_size ? page_size : 0);
	_heap.data.truncate(size);
}

template char *json_skip_whitespace(char *, char *);
template char *json_str_write_codepoint(char *, unsigned int);
template int json_parse_hex(char *, char *);
template json_document json_parse(char *, char *);

std::ostream &operator<<(std::ostream &out, const json_string &x) {
	out.write(x.c_str(), x.size());
	return out;
}

template std::ostream &operator<<(std::ostream &, const json_value_imp<json_allocator<char>> &);
template std::ostream &operator<<(std::ostream &, const json_value_imp<std::allocator<void>> &);
