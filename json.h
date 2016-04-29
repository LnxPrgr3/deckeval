#ifndef DECKEVAL_JSON_H
#define DECKEVAL_JSON_H
#include <exception>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <vector>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include "mapping.h"

class json_exception : public std::exception {
public:
	json_exception(const char *msg) : _what(msg) {}
	const char *what() const noexcept { return _what; }
private:
	const char * const _what;
};

struct json_allocator_heap {
	mapping::options options;
	mapping data;
	size_t pos;

	json_allocator_heap() { }
	json_allocator_heap(size_t n);

	json_allocator_heap &operator=(json_allocator_heap &&x) {
		options = x.options;
		data = std::move(x.data);
		pos = x.pos;
		return *this;
	}
};

class json_allocator_base {
public:
	json_allocator_base() : _heap(nullptr) { }
	json_allocator_base(json_allocator_heap *heap) : _heap(heap) { }
	void *allocate(size_t n, size_t alignment);
protected:
	json_allocator_heap *_heap;
};

template <class T>
class json_allocator : public json_allocator_base {
public:
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef void *void_pointer;
	typedef const void *const_void_pointer;
	typedef T &reference;
	typedef const T &const_reference;
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	template <class U>
	struct rebind {
		typedef json_allocator<U> other;
	};
	json_allocator() { }
	json_allocator(json_allocator_heap *heap) : json_allocator_base(heap) { }
	template <class U>
	json_allocator(const json_allocator<U> &x) : json_allocator_base(x._heap) { }
	pointer allocate(size_t n) {
		return (pointer) json_allocator_base::allocate(n*sizeof(T), alignof(T));
	}
	template <class U, class ...Args>
	void construct(U *p, Args&&... args) {
		new ((void *)p) U(std::forward<Args>(args)...);
	}
	template <class U>
	void destroy(U *p) {
		p->~U();
	}
	void deallocate(pointer ptr, size_t n) { }
	json_allocator &operator=(const json_allocator &x) {
		_heap = x._heap;
		return *this;
	}
	template <class U>
	bool operator==(const json_allocator<U> &x) { return _heap == x._heap; }
	template <class U>
	bool operator!=(const json_allocator<U> &x) { return _heap != x._heap; }

	template <class U>
	friend class json_allocator;
};

class json_string {
public:
	json_string(const char *data, size_t len) : _data(data), _len(len) {}
	const char *c_str() const { return _data; }
	size_t size() const { return _len; }
	bool operator<(const json_string &x) const;
	bool operator==(const char *x);
private:
	const char *_data;
	size_t _len;
};

class json_object {};
class json_array {};
class json_end {};

class json_value_base {
protected:
	enum {NONE, BOOLEAN, NUMBER, STRING, OBJECT, ARRAY, END} _type;
public:
	json_value_base(decltype(_type) type) : _type(type) { }
	bool is_null() const { return _type == NONE; }
	bool is_boolean() const { return _type == BOOLEAN; }
	bool is_number() const { return _type == NUMBER; }
	bool is_string() const { return _type == STRING; }
	bool is_object() const { return _type == OBJECT; }
	bool is_array() const { return _type == ARRAY; }
	bool is_end() const { return _type == END; }
};

template <class Allocator>
class json_value_imp : public json_value_base {
public:
	typedef typename Allocator::template rebind<std::pair<json_string, json_value_imp>>::other object_allocator;
	typedef typename Allocator::template rebind<json_value_imp>::other array_allocator;
	typedef std::map<json_string, json_value_imp, std::less<json_string>, object_allocator> object;
	typedef std::list<json_value_imp, array_allocator> array;

	class obj {
	public:
		auto begin() -> decltype(object().cbegin()) const {
			return _parent._object.cbegin();
		}
		auto end() -> decltype(object().cend()) const {
			return _parent._object.cend();
		}
		friend class json_value_imp;
	private:
		obj(const json_value_imp *x) : _parent(*x) {}
		const json_value_imp &_parent;
	};

	class arr {
	public:
		auto begin() -> decltype(array().cbegin()) const {
			return _parent._array.cbegin();
		}
		auto end() -> decltype(array().cend()) const {
			return _parent._array.cend();
		}
		friend class json_value_imp;
	private:
		arr(const json_value_imp *x) : _parent(*x) {}
		const json_value_imp &_parent;
	};

	json_value_imp() : json_value_base(NONE) {}
	json_value_imp(const json_value_imp &x) : json_value_base(NONE) {
		*this = x;
	}
	json_value_imp(bool value) : json_value_base(BOOLEAN), _boolean(value) {}
	json_value_imp(double value) : json_value_base(NUMBER), _number(value) {}
	json_value_imp(const char *start, const char *end) : json_value_base(STRING), _string(start, end-start) {}
	json_value_imp(const json_object &, const Allocator &allocator = Allocator()) : json_value_base(OBJECT), _object(allocator) {}
	json_value_imp(const json_array &, const Allocator &allocator = Allocator()) : json_value_base(ARRAY), _array(allocator) {}
	json_value_imp(const json_end &) : json_value_base(END) {}
	~json_value_imp() {
		if(is_object())
			_object.~map();
		else if(is_array())
			_array.~list();
	}
	json_value_imp &operator=(const json_value_imp &x) {
		_type = x._type;
		switch(_type) {
		case NONE:
			break;
		case BOOLEAN:
			_boolean = x._boolean;
			break;
		case NUMBER:
			_number = x._number;
			break;
		case STRING:
			new(&_string) json_string(x._string);
			break;
		case OBJECT:
			new(&_object) object(x._object);
			break;
		case ARRAY:
			new(&_array) array(x._array);
			break;
		case END:
			break;
		}
		return *this;
	}
	json_value_imp &operator=(json_value_imp &&x) {
		_type = x._type;
		switch(_type) {
		case NONE:
			break;
		case BOOLEAN:
			_boolean = x._boolean;
			break;
		case NUMBER:
			_number = x._number;
			break;
		case STRING:
			new(&_string) json_string(x._string);
			break;
		case OBJECT:
			new(&_object) object(std::move(x._object));
			break;
		case ARRAY:
			new(&_array) array(std::move(x._array));
			break;
		case END:
			break;
		}
		return *this;
	}
	bool as_boolean() const {
		if(!is_boolean())
			throw json_exception("Invalid conversion to boolean");
		return _boolean;
	}
	double as_number() const {
		if(!is_number())
			throw json_exception("Invalid conversion to double");
		return _number;
	}
	json_string as_string() const {
		if(!is_string())
			throw json_exception("Invalid conversion to string");
		return _string;
	}
	obj as_object() const {
		if(!is_object())
			throw json_exception("Invalid conversion to object");
		return obj(this);
	}
	arr as_array() const {
		if(!is_array())
			throw json_exception("Invalid conversion to array");
		return arr(this);
	}
	json_value_imp &insert(const json_string &key, const json_value_imp &value) {
		if(!is_object())
			throw json_exception("Invalid conversion to object");
		auto &rv = _object[key];
		rv = value;
		return rv;
	}
	json_value_imp &insert(const json_value_imp &value) {
		if(!is_array())
			throw json_exception("Invalid conversion to array");
		_array.push_back(value);
		return _array.back();
	}
	bool has_key(const char *k) const {
		if(!is_object())
			throw json_exception("Invalid conversion to object");
		json_string key(k, strlen(k));
		auto x = _object.find(key);
		if(x == _object.end())
			return false;
		return true;
	}
	const json_value_imp &operator[](const char *k) const {
		if(!is_object())
			throw json_exception("Invalid conversion to object");
		json_string key(k, strlen(k));
		auto x = _object.find(key);
		if(x == _object.end())
			throw json_exception("Key not found");
		return x->second;
	}
protected:
	union {
		bool _boolean;
		double _number;
		json_string _string;
		object _object;
		array _array;
	};
};

extern template class json_value_imp<std::allocator<void>>;
extern template class json_value_imp<json_allocator<char>>;

typedef json_value_imp<std::allocator<void>> json_value;

class json_document : public json_value_imp<json_allocator<char>> {
public:
	typedef json_value_imp<json_allocator<char>> value_type;
	json_document() { }
	json_document(size_t n) : _heap(n), _allocator(&_heap) { }
	json_document(const json_document &) = delete;
	json_document(json_document &&x) {
		_heap = std::move(x._heap);
		_allocator = x._allocator;
	}
	~json_document();
	value_type convert(const json_value &x);
	json_document &operator=(json_document &&x) {
		_heap = std::move(x._heap);
		_allocator = x._allocator;
		value_type::operator=(std::move(x));
		return *this;
	}
	json_document &operator=(const value_type &x) {
		value_type::operator=(x);
		return *this;
	}
	void shrink_to_fit();
private:
	json_allocator_heap _heap;
	json_allocator<char> _allocator;
};

template <class ForwardIterator>
ForwardIterator json_skip_whitespace(ForwardIterator begin, ForwardIterator end) {
	while(begin != end) {
		switch(*begin) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			break;
		default:
			return begin;
		}
		++begin;
	}
	return begin;
}

template <class ForwardIterator>
void json_nonempty(ForwardIterator begin, ForwardIterator end) {
	if(begin == end)
		throw json_exception("Unexpected end of input");
}

template <class ForwardIterator>
ForwardIterator json_parse_char(ForwardIterator begin, ForwardIterator end, char c) {
	if(begin != end && *begin == c)
		return ++begin;
	throw json_exception("");
}

template <class ForwardIterator, class Callback>
ForwardIterator json_parse_boolean(ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	try {
		switch(*begin) {
		case 't':
			++begin;
			begin = json_parse_char(begin, end, 'r');
			begin = json_parse_char(begin, end, 'u');
			begin = json_parse_char(begin, end, 'e');
			begin = json_skip_whitespace(begin, end);
			cb(json_value(true));
			break;
		case 'f':
			++begin;
			begin = json_parse_char(begin, end, 'a');
			begin = json_parse_char(begin, end, 'l');
			begin = json_parse_char(begin, end, 's');
			begin = json_parse_char(begin, end, 'e');
			cb(json_value(true));
			break;
		}
		return begin;
	} catch (json_exception &e) {
		throw json_exception("Unexpected bare word");
	}
}


template <class ForwardIterator, class Callback>
ForwardIterator json_parse_number(ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	long number = 0;
	long frac = 0;
	long exp = 0;
	int sign = 1;
	int fracdigits = 0;
	int expsign = 1;
	json_nonempty(begin, end);
	if(*begin == '-') {
		sign = -1;
		json_nonempty(++begin, end);
	}
	if(*begin != '0') {
		while(begin != end && *begin >= '0' && *begin <= '9') {
			number = number*10 + (*begin - '0');
			++begin;
		}
	} else
		++begin;
	if(begin != end && *begin == '.') {
		++begin;
		while(begin != end && *begin >= '0' && *begin <= '9') {
			frac = frac * 10 + (*begin - '0');
			++fracdigits;
			++begin;
		}
	}
	if(begin != end && (*begin == 'e' || *begin == 'E')) {
		++begin;
		if(begin != end && *begin == '+')
			++begin;
		else if(begin != end && *begin == '-') {
			++begin;
			expsign = -1;
		}
		json_nonempty(begin, end);
		while(begin != end && *begin >= '0' && *begin <= '9') {
			exp = exp*10 + (*begin - '0');
			++begin;
		}
	}
	cb(json_value(sign*(number+frac*pow(0.1, fracdigits))*pow(10, expsign*exp)));
	return begin;
}

template <class ForwardIterator>
ForwardIterator json_str_write(ForwardIterator pos, char c) {
	*pos = c;
	return ++pos;
}

template <class ForwardIterator>
ForwardIterator json_str_write_codepoint(ForwardIterator pos, unsigned int codepoint) {
	if(codepoint < 0b10000000) {
		pos = json_str_write(pos, codepoint);
	} else {
		int len = codepoint < 0x800 ? 2 :
			      codepoint < 0x10000 ? 3 :
			      codepoint < 0x200000 ? 4 :
			      codepoint < 0x4000000 ? 5 :
			      6;
		codepoint = codepoint << (32 - 5 * len - 1);
		pos = json_str_write(pos,  (0xffu << (8 - len)) | ((codepoint & (0xffu << (32 - (7 - len)))) >> (24 + len + 1)));
		codepoint <<= 7 - len;
		switch(len) {
		case 6:
			pos = json_str_write(pos,  0b10000000 | ((codepoint & 0xfc000000) >> 26));
			codepoint <<= 6;
		case 5:
			pos = json_str_write(pos,  0b10000000 | ((codepoint & 0xfc000000) >> 26));
			codepoint <<= 6;
		case 4:
			pos = json_str_write(pos,  0b10000000 | ((codepoint & 0xfc000000) >> 26));
			codepoint <<= 6;
		case 3:
			pos = json_str_write(pos,  0b10000000 | ((codepoint & 0xfc000000) >> 26));
			codepoint <<= 6;
		case 2:
			pos = json_str_write(pos,  0b10000000 | ((codepoint & 0xfc000000) >> 26));
		}
	}
	return pos;
}

template <class ForwardIterator>
int json_parse_hex(ForwardIterator begin, ForwardIterator end) {
	json_nonempty(begin, end);
	if(*begin >= '0' && *begin <= '9')
		return *begin - '0';
	else if(*begin >= 'a' && *begin <= 'f')
		return 10 + *begin - 'a';
	else if(*begin >= 'A' && *begin <= 'F')
		return 10 + *begin - 'A';
	else
		throw json_exception("Unexpected character in unicode escape sequence");
}

template <class ForwardIterator, class Callback>
ForwardIterator json_parse_string_slow(ForwardIterator str, ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	ForwardIterator wpos = begin;
	while(begin != end && *begin != '"') {
		if(*begin == '\\') {
			unsigned int codepoint = 0;
			json_nonempty(++begin, end);
			switch(*begin) {
			case '"':
			case '\\':
			case '/':
				wpos = json_str_write(wpos, *begin);
				break;
			case 'b':
				wpos = json_str_write(wpos, '\b');
				break;
			case 'f':
				wpos = json_str_write(wpos, '\f');
				break;
			case 'n':
				wpos = json_str_write(wpos, '\n');
				break;
			case 'r':
				wpos = json_str_write(wpos, '\r');
				break;
			case 't':
				wpos = json_str_write(wpos, '\t');
				break;
			case 'u':
				codepoint = json_parse_hex(++begin, end);
				codepoint = codepoint * 16 + json_parse_hex(++begin, end);
				codepoint = codepoint * 16 + json_parse_hex(++begin, end);
				codepoint = codepoint * 16 + json_parse_hex(++begin, end);
				wpos = json_str_write_codepoint(wpos, codepoint);
				break;
			default:
				throw json_exception("Unrecognized string escape sequence");
			}
		} else
			wpos = json_str_write(wpos, *begin);
		++begin;
	}
	json_nonempty(begin, end);
	cb(json_value(str, wpos));
	return ++begin;
}

template <class ForwardIterator, class Callback>
ForwardIterator json_parse_string(ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	ForwardIterator str = begin;
	while(begin != end && *begin != '"') {
		if(*begin == '\\')
			return json_parse_string_slow(str, begin, end, cb);
		++begin;
	}
	json_nonempty(begin, end);
	cb(json_value(str, begin));
	return ++begin;
}

template <class ForwardIterator, class Callback>
ForwardIterator json_parse(ForwardIterator begin, ForwardIterator end, Callback &&cb);

template <class ForwardIterator, class Callback>
ForwardIterator json_parse_object(ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	cb(json_value(json_object()));
	begin = json_skip_whitespace(begin, end);
	json_nonempty(begin, end);
	while(*begin != '}') {
		if(*begin != '"')
			throw json_exception("Invalid object key");
		begin = json_parse_string(++begin, end, cb);
		begin = json_skip_whitespace(begin, end);
		json_nonempty(begin, end);
		begin = json_parse_char(begin, end, ':');
		begin = json_skip_whitespace(begin, end);
		begin = json_parse(begin, end, cb);
		begin = json_skip_whitespace(begin, end);
		json_nonempty(begin, end);
		if(*begin == ',') {
			begin = json_skip_whitespace(++begin, end);
			json_nonempty(begin, end);
		} else if(*begin != '}')
			throw json_exception("Object key/value pairs must be separated by commas");
	}
	cb(json_value(json_end()));
	return ++begin;
}

template <class ForwardIterator, class Callback>
ForwardIterator json_parse_array(ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	cb(json_value(json_array()));
	begin = json_skip_whitespace(begin, end);
	json_nonempty(begin, end);
	while(*begin != ']') {
		begin = json_parse(begin, end, cb);
		begin = json_skip_whitespace(begin, end);
		json_nonempty(begin, end);
		if(*begin == ',') {
			begin = json_skip_whitespace(++begin, end);
			json_nonempty(begin, end);
		} else if(*begin != ']')
			throw json_exception("Array members must be separated by commas");
	}
	cb(json_value(json_end()));
	return ++begin;
}

template <class ForwardIterator, class Callback>
ForwardIterator json_parse(ForwardIterator begin, ForwardIterator end, Callback &&cb) {
	begin = json_skip_whitespace(begin, end);
	json_nonempty(begin, end);
	switch(*begin) {
	case 'n':
		++begin;
		begin = json_parse_char(begin, end, 'u');
		begin = json_parse_char(begin, end, 'l');
		begin = json_parse_char(begin, end, 'l');
		cb(json_value());
		break;
	case 't':
	case 'f':
		begin = json_parse_boolean(begin, end, cb);
		break;
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		begin = json_parse_number(begin, end, cb);
		break;
	case '"':
		begin = json_parse_string(++begin, end, cb);
		break;
	case '{':
		begin = json_parse_object(++begin, end, cb);
		break;
	case '[':
		begin = json_parse_array(++begin, end, cb);
		break;
	}
	return begin;
}

template <class ForwardIterator>
json_document json_parse(ForwardIterator begin, ForwardIterator end) {
	json_document rv((end-begin)*sizeof(void *));
	json_value key;
	std::vector<json_document::value_type *> stack;
	bool root_set = false, key_set = false;
	json_parse(begin, end, [&](const json_value &x) {
		if(!root_set) {
			rv = rv.convert(x);
			root_set = true;
			if(x.is_object() || x.is_array()) {
				stack.push_back(&rv);
			}
			return;
		}
		if(x.is_end()) {
			stack.pop_back();
			return;
		}
		auto cur = stack.back();
		if(cur->is_object()) {
			if(!key_set) {
				key = x;
				key_set = true;
			} else {
				key_set = false;
				auto &val = cur->insert(key.as_string(), rv.convert(x));
				if(val.is_array() || val.is_object()) {
					stack.push_back(&val);
				}
			}
		} else {
			auto &val = cur->insert(rv.convert(x));
			if(val.is_array() || val.is_object()) {
				stack.push_back(&val);
			}
		}
	});
	rv.shrink_to_fit();
	return rv;
}

extern template char *json_skip_whitespace(char *, char *);
extern template char *json_str_write_codepoint(char *, unsigned int);
extern template int json_parse_hex(char *, char *);
extern template json_document json_parse(char *, char *);

std::ostream &operator<<(std::ostream &out, const json_string &x);

template <class Allocator>
std::ostream &operator<<(std::ostream &out, const json_value_imp<Allocator> &x) {
	if(x.is_null())
		out << "null";
	else if(x.is_boolean())
		out << x.as_boolean();
	else if(x.is_number())
		out << x.as_number();
	else if(x.is_string())
		out << x.as_string();
	else if(x.is_object())
		out << "[object]";
	else if(x.is_array())
		out << "[array]";
	return out;
}

extern template std::ostream &operator<<(std::ostream &, const json_value_imp<json_allocator<char>> &);
extern template std::ostream &operator<<(std::ostream &, const json_value_imp<std::allocator<void>> &);

#endif
