#ifndef DECKEVAL_JSON_H
#define DECKEVAL_JSON_H
#include <iostream>
#include <new>
#include <cstring>
#include <cstddef>
#include <sstream>
#include "mapping.h"

class json_boolean;
class json_number;
class json_string;
class json_array;
class json_object;
class json_var;

class json_value {
public:
	virtual ~json_value();
	__attribute__((pure)) virtual operator json_boolean() const = 0;
	__attribute__((pure)) virtual operator json_number() const = 0;
	__attribute__((pure)) virtual operator json_string() const = 0;
	__attribute__((pure)) virtual operator json_array() const = 0;
	__attribute__((pure)) virtual operator json_object() const = 0;
};

class json_null : public json_value {
public:
	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
};

class json_boolean : public json_value {
public:
	json_boolean(bool value) : _value{value} { }
	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
	__attribute__((pure)) operator bool() const { return _value; }
private:
	bool _value;
};

class json_number : public json_value {
public:
	json_number(double value) : _value(value) { }
	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
	__attribute__((pure)) operator double() const { return _value; }
private:
	double _value;
};

class json_string : public json_value {
public:
	struct extent {
		union {
			const char *_value;
			char _data[sizeof(char *)];
		};
		size_t _size:(sizeof(size_t)*8-2),
		       _internal:1;
		extent *_next;

		extent(json_string &&x) {
			_value = x._value;
			_size = x._size;
			_internal = false;
			_next = nullptr;
			x._value = nullptr;
			x._size = 0;
			x._dynamic = false;
			x._multipart = false;
			_next = nullptr;
		}

		extent(size_t size) {
			_size = size;
			_internal = true;
			_next = nullptr;
		}

		const char *value() const { return _internal ? _data : _value; }
	};
	class const_iterator {
	public:
		const_iterator(const const_iterator &x) : _pos(x._pos), _end(x._end), _next(x._next) { }
		const_iterator &operator++() {
			++_pos;
			while(_pos && _pos == _end) {
				if(!_next)
					_pos = nullptr;
				else {
					_pos = _next->value();
					_end = _pos+_next->_size;
					_next = _next->_next;
				}
			}
			return *this;
		}
		const_iterator operator++(int) {
			const_iterator rv = *this;
			++*this;
			return rv;
		}
		const char &operator*() const { return *_pos; }
		bool operator!=(const const_iterator &x) { return _pos != x._pos; }
		bool operator==(const const_iterator &x) { return _pos == x._pos; }
	private:
		const_iterator(const char *pos, const char *end, const extent *next) : _pos(pos), _end(end), _next(next) { }
		const char *_pos;
		const char *_end;
		const extent *_next;

		friend class json_string;
	};
	json_string(const char *value, size_t size, bool dynamic) {
		if(dynamic) {
			char *str = new char[size];
			memcpy(str, value, size);
			_value = str;
			_dynamic = true;
			_multipart = false;
			_size = size;
		} else {
			_value = value;
			_dynamic = false;
			_multipart = false;
			_size = size;
		}
	}
	json_string(const char *value, size_t size) : json_string(value, size, false) { }
	json_string(const char *value) : json_string(value, strlen(value)) { }
	json_string(const json_string &x) {
		if(!x._multipart) {
			if(x._dynamic) {
				char *str = new char[x._size];
				memcpy(str, x._value, x._size);
				_value = str;
				_dynamic = true;
				_multipart = false;
				_size = x._size;
			} else {
				_value = x._value;
				_dynamic = false;
				_multipart = false;
				_size = x._size;
			}
		} else {
			_next = x._next;
			_size = x._size;
			_dynamic = false;
			_multipart = true;
		}
	}
	json_string(json_string &&x) {
		_value = x._value;
		_dynamic = x._dynamic;
		_multipart = x._multipart;
		x._value = nullptr;
		_size = x._size;
		x._dynamic = false;
		x._multipart = false;
		x._size = 0;
	}
	~json_string() {
		if(_dynamic) {
			char *str = const_cast<char *>(_value);
			delete[] str;
		}
	}
	json_string operator=(const json_string &x) {
		if(this != &x) {
			this->~json_string();
			new(this) json_string(x);
		}
		return *this;
	}
	bool empty() const { return !_size && !_multipart; }
	bool is_null() const { return !_multipart && !_value; }
	const_iterator begin() const {
		if(!_multipart) {
			if(_size)
				return const_iterator(_value, _value+_size, nullptr);
			else
				return end();
		}
		return const_iterator(_next->value(), _next->value()+_next->_size, _next->_next);
	}
	const_iterator end() const {
		return const_iterator(nullptr, nullptr, nullptr);
	}
	operator std::string() const {
		if(!_multipart)
			return std::string(_value, _size);
		else {
			std::stringstream rv;
			for(extent *next = _next; next; next = next->_next) {
				rv.write(next->value(), next->_size);
			}
			return rv.str();
		}
	}
	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
	bool operator==(const json_string &x) const {
		if(!_multipart)
			return _size == x._size && memcmp(_value, x._value, _size) == 0;
		auto src = begin();
		auto dst = x.begin();
		while(src != end()) {
			if(dst == x.end())
				return false;
			if(*src != *dst)
				return false;
			++src;
			++dst;
		}
		return dst == x.end();
	}
private:
	union {
		const char *_value;
		extent *_next;
	};
	size_t _size:(sizeof(size_t)*8-2),
	       _dynamic:1,
	       _multipart:1;
	
	friend std::ostream &operator<<(std::ostream &, const json_string &);
	template <class Allocator> friend class json_string_imp;
	friend class std::hash<json_string>;
};

inline std::ostream &operator<<(std::ostream &out, const json_string &x) {
	if(!x._multipart) {
		out.write(x._value, x._size);
	} else {
		for(json_string::extent *next = x._next; next; next = next->_next) {
			out.write(next->value(), next->_size);
		}
	}
	return out;
}

template <class Allocator = std::allocator<json_string::extent>>
class json_string_imp : public json_string {
public:
	json_string_imp(const char *value, size_t size, bool dynamic, const Allocator &allocator = Allocator()) : json_string(value, size, dynamic), _allocator(allocator) { }
	json_string_imp(const char *value, size_t size, const Allocator allocator = Allocator()) : json_string_imp(value, size, false, allocator) { }
	json_string_imp(const char *value, const Allocator allocator = Allocator()) : json_string_imp(value, strlen(value), allocator) { }
	void append(json_string &&x) {
		make_multipart();
		extent *next = _allocator.allocate(1);
		_allocator.construct(next, std::move(x));
		*_tail = next;
		_tail = &next->_next;
	}
	char *append_internal(size_t size) {
		make_multipart();
		extent *next = _allocator.allocate(1);
		_allocator.construct(next, size);
		*_tail = next;
		_tail = &next->_next;
		return next->_data;
	}
private:
	void make_multipart() {
		if(!_multipart) {
			if(_size) {
				extent *first = _allocator.allocate(1);
				_allocator.construct(first, std::move(*this));
				_next = first;
				_tail = &first->_next;
			} else {
				_value = nullptr;
				_tail = &_next;
			}
			_multipart = true;
		}
	}
	Allocator _allocator;
	extent **_tail;
};

template <class Allocator>
class json_array_imp;

template <class Allocator>
class json_object_imp;

class json_array : public json_value {
protected:
	class node;
public:
	class const_iterator {
	public:
		const_iterator(const const_iterator &x) : _pos(x._pos) { }
		const_iterator &operator++();
		const json_var &operator*() const;
		const json_var *operator->() const;
		bool operator!=(const const_iterator &x) const;
		bool operator==(const const_iterator &x) const;
	protected:
		const_iterator(node *pos) : _pos(pos) { }
		const node *_pos;

		friend class json_array;
	};

	class iterator : public const_iterator {
	public:
		iterator(const iterator &x) : const_iterator(x) { }
		json_var &operator*();
		json_var *operator->();
	private:
		iterator(node *pos) : const_iterator(pos) { }

		friend class json_array;
	};

	json_array() : _head(nullptr), _size(0) { }
	json_array(const json_array &x) = default;
	json_array(json_array &&x) : _head(x._head), _size(x._size) {
		x._head = nullptr;
		x._size = 0;
	}
	virtual ~json_array() { }

	const_iterator begin() const { return const_iterator(_head); }
	iterator begin() { return iterator(_head); }
	const_iterator end() const { return const_iterator(nullptr); }
	iterator end() { return iterator(nullptr); }
	const size_t size() const { return _size; }
	bool contains(const json_string &x) const;

	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
protected:
	node *_head;
	size_t _size;
};

template <class Allocator = std::allocator<json_var>>
class json_array_imp : public json_array {
public:
	json_array_imp(const Allocator &allocator = Allocator()) : _allocator(allocator), _tail(&_head) { }
	json_array_imp(json_array_imp &&x) : _allocator(x._allocator), _tail(x._tail == &x._head ? &_head : x._tail), json_array(std::move(x)) { }
	json_array_imp(json_array &&x, const Allocator allocator = Allocator()) : json_array(std::move(x)), _allocator(allocator), _tail(nullptr) { }
	~json_array_imp();
	void push_back(const json_null &value);
	void push_back(const json_boolean &value);
	void push_back(const json_number &value);
	void push_back(const json_string &value);
	void push_back(const json_value &value);
	void push_back(const json_var &value);
	template <class A2>
	void push_back(json_array_imp<A2> &&value);
	template <class A2>
	void push_back(json_object_imp<A2> &&object);
private:
	typedef typename Allocator::template rebind<node>::other allocator;
	node **_tail;
	allocator _allocator;

	friend class json_array;
};

class json_object : public json_value {
protected:
	class node;
public:
	class const_iterator {
	public:
		const_iterator(const const_iterator &x) : _pos(x._pos) { }
		const_iterator &operator++();
		const std::pair<json_string, json_var> &operator*() const;
		const std::pair<json_string, json_var> *operator->() const;
		bool operator!=(const const_iterator &x) const;
		bool operator==(const const_iterator &x) const;
	protected:
		const_iterator(node *pos) : _pos(pos) { }
		const node *_pos;

		friend class json_object;
	};

	class iterator : public const_iterator {
	public:
		iterator(const iterator &x) : const_iterator(x) { }
		std::pair<json_string, json_var> &operator*();
		std::pair<json_string, json_var> *operator->();
	private:
		iterator(node *pos) : const_iterator(pos) { }

		friend class json_object;
	};

	json_object() : _head(nullptr), _index(nullptr) { }
	json_object(const json_object &x) = default;
	json_object(json_object &&x) : _head(x._head), _index(x._index) {
		x._head = nullptr;
		x._index = nullptr;
	}
	virtual ~json_object() { }

	const_iterator begin() const { return const_iterator(_head); }
	iterator begin() { return iterator(_head); }
	const_iterator end() const { return const_iterator(nullptr); }
	iterator end() { return iterator(nullptr); }

	bool has_key(const json_string &key) const;
	const json_var &operator[](const json_string &key) const;
	json_var &operator[](const json_string &key);

	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
protected:
	void index(size_t buckets);
	node *_head;
	node **_index;
};

template <class Allocator = std::allocator<std::pair<json_string, json_var>>>
class json_object_imp : public json_object {
public:
	json_object_imp(const Allocator &allocator = Allocator()) : _allocator(allocator), _tail(&_head), _size(0) { }
	json_object_imp(json_object_imp &&x) : _allocator(x._allocator), _tail(x._tail == &x._head ? &_head : x._tail), _size(x._size), json_object(std::move(x)) { }
	json_object_imp(json_object &&x, const Allocator &allocator = Allocator()) : json_object(std::move(x)), _allocator(allocator), _tail(nullptr), _size(0) { }
	~json_object_imp();
	void push_back(const json_string &key, const json_null &value);
	void push_back(const json_string &key, const json_boolean &value);
	void push_back(const json_string &key, const json_number &value);
	void push_back(const json_string &key, const json_string &value);
	void push_back(const json_string &key, const json_value &value);
	void push_back(const json_string &key, const json_var &value);
	template <class A2>
	void push_back(const json_string &key, json_array_imp<A2> &&value);
	template <class A2>
	void push_back(const json_string &key, json_object_imp<A2> &&object);
	void index();
private:
	typedef typename Allocator::template rebind<node>::other allocator;
	node **_tail;
	size_t _size;
	allocator _allocator;

	friend class json_object;
};

template <class Allocator>
class json_var_imp;

class json_var : public json_value {
public:
	json_var() : _type(NONE), _null() { }
	json_var(const json_null &x) : _type(NONE), _null() { }
	json_var(const json_boolean &x) : _type(BOOLEAN), _boolean(x) { }
	json_var(const json_number &x) : _type(NUMBER), _number(x) { }
	json_var(const json_string &x) : _type(STRING), _string(x) { }
	json_var(const json_value &x);
	json_var(const json_var &x);
	template <class Allocator>
	json_var(json_array_imp<Allocator> &&x) {
		_type = ARRAY;
		new ((void *)&_array) json_array(std::move(x));
	}
	template <class Allocator>
	json_var(json_object_imp<Allocator> &&x) {
		_type = OBJECT;
		x.index();
		new ((void *)&_object) json_object(std::move(x));
	}
	json_var(double x) {
		_type = NUMBER;
		new ((void *)&_number) json_number(x);
	}
	json_var(const char *x) {
		_type = STRING;
		new ((void *)&_string) json_string(x);
	}
	virtual ~json_var();
	json_var &operator=(const json_var &x);
	__attribute__((pure)) const json_value &value() const;
	__attribute__((pure)) operator json_boolean() const;
	__attribute__((pure)) operator json_number() const;
	__attribute__((pure)) operator json_string() const;
	__attribute__((pure)) operator json_array() const;
	__attribute__((pure)) operator json_object() const;
private:
	void destroy_subobject();
	enum {NONE, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT} _type;
	union {
		json_null _null;
		json_boolean _boolean;
		json_number _number;
		json_string _string;
		json_array _array;
		json_object _object;
	};

	template <class Allocator> friend class json_var_imp;
};

template <class Allocator = std::allocator<void>>
class json_var_imp : public json_var {
public:
	json_var_imp(const json_value &x, const Allocator &allocator = Allocator()) : json_var(x), _allocator(allocator) { }
	json_var_imp(const json_var &x, const Allocator &allocator = Allocator()) : json_var(x), _allocator(allocator) { }
	json_var_imp(const json_var_imp &x, const Allocator &allocator = Allocator()) : json_var(x), _allocator(allocator) { }
	json_var_imp(json_array_imp<typename Allocator::template rebind<json_var>::other> &&x, const Allocator &allocator = Allocator()) : json_var(std::move(x)), _allocator(allocator) { }
	json_var_imp(json_object_imp<typename Allocator::template rebind<std::pair<json_string, json_var>>::other> &&x, const Allocator &allocator = Allocator()) : json_var(std::move(x)), _allocator(allocator) { }
	virtual ~json_var_imp() {
		destroy(*this);
	}
private:
	void destroy(json_var &x) {
		if(x._type == ARRAY) {
			typedef typename Allocator::template rebind<json_var>::other A2;
			json_array_imp<A2> arr(std::move(x._array), _allocator);
			for(auto &val: arr)
				destroy(val);
		} else if(x._type == OBJECT) {
			typedef typename Allocator::template rebind<std::pair<json_string, json_var>>::other A2;
			json_object_imp<A2> obj(std::move(x._object), _allocator);
			for(auto &val: obj)
				destroy(val.second);
		}
	}
	Allocator _allocator;
};

extern const json_var json_none;

struct json_array::node {
	json_var value;
	node *next;

	node(const json_null &value) : value(value), next(nullptr) { }
	node(const json_boolean &value) : value(value), next(nullptr) { }
	node(const json_number &value) : value(value), next(nullptr) { }
	node(const json_string &value) : value(value), next(nullptr) { }
	node(const json_value &value) : value(value), next(nullptr) { }
	template <class Allocator>
	node(json_array_imp<Allocator> &&value) : value(std::move(value)), next(nullptr) { }
	template <class Allocator>
	node(json_object_imp<Allocator> &&value) : value(std::move(value)), next(nullptr) { }
};

struct json_object::node {
	std::pair<json_string, json_var> value;
	node *next;
	node *hash_next;

	node(const json_string &key, const json_null &value) : value(key, value), next(nullptr), hash_next(nullptr) { }
	node(const json_string &key, const json_boolean &value) : value(key, value), next(nullptr), hash_next(nullptr) { }
	node(const json_string &key, const json_number &value) : value(key, value), next(nullptr), hash_next(nullptr) { }
	node(const json_string &key, const json_string &value) : value(key, value), next(nullptr), hash_next(nullptr) { }
	node(const json_string &key, const json_value &value) : value(key, value), next(nullptr), hash_next(nullptr) { }
	template <class Allocator>
	node(const json_string &key, json_array_imp<Allocator> &&value) : value(key, std::move(value)), next(nullptr), hash_next(nullptr) { }
	template <class Allocator>
	node(const json_string &key, json_object_imp<Allocator> &&value) : value(key, std::move(value)), next(nullptr), hash_next(nullptr) { }
};

struct json_allocator_heap {
	mapping::options options;
	mapping data;
	size_t pos;

	json_allocator_heap() { }
	json_allocator_heap(json_allocator_heap &&x) {
		this->operator=(std::move(x));
	}
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

class json_document : public json_var {
public:
	json_document() { }
	json_document(size_t n) : _heap(n), _allocator(&_heap) { }
	json_document(const json_document &) = delete;
	json_document(json_document &&x) : _heap(std::move(x._heap)), _allocator(&_heap) {
		json_var::operator=(std::move(x));
	}
	json_document &operator=(json_var &&x) {
		json_var::operator=(std::move(x));
		return *this;
	}
	void shrink_to_fit();
private:
	json_allocator_heap _heap;
	json_allocator<char> _allocator;

	friend class json_parse_callbacks;
	friend json_document json_parse(const char *, const char *);
};

class json_exception : public std::exception {
public:
	json_exception(const char *msg) : _what(msg) {}
	const char *what() const noexcept { return _what; }
private:
	const char * const _what;
};

class json_callbacks {
public:
	virtual void null() = 0;
	virtual void boolean(const json_boolean &value) = 0;
	virtual void number(const json_number &value) = 0;
	virtual void string(const json_string &value) = 0;
	virtual void array_begin() = 0;
	virtual void array_end() = 0;
	virtual void object_begin() = 0;
	virtual void object_end() = 0;
};

const char *json_parse(const char *begin, const char *end, json_callbacks &cb);
json_document json_parse(const char *begin, const char *end);

template <class allocator>
json_array_imp<allocator>::~json_array_imp() {
	while(_head) {
		node *next = _head->next;
		_allocator.destroy(_head);
		_allocator.deallocate(_head, 1);
		_head = next;
	}
}

template <class Allocator>
void json_array_imp<Allocator>::push_back(const json_null &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_array_imp<Allocator>::push_back(const json_boolean &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_array_imp<Allocator>::push_back(const json_number &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_array_imp<Allocator>::push_back(const json_string &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_array_imp<Allocator>::push_back(const json_value &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_array_imp<Allocator>::push_back(const json_var &value) {
	push_back(value.value());
}

template<class Allocator>
template <class A2>
void json_array_imp<Allocator>::push_back(json_array_imp<A2> &&value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, std::move(value));
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template<class Allocator>
template <class A2>
void json_array_imp<Allocator>::push_back(json_object_imp<A2> &&value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, std::move(value));
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
json_object_imp<Allocator>::~json_object_imp() {
	while(_head) {
		node *next = _head->next;
		_allocator.destroy(_head);
		_allocator.deallocate(_head, 1);
		_head = next;
	}
	if(_index) {
		typename Allocator::template rebind<node *>::other a2(_allocator);
		size_t buckets = (size_t)_index[0];
		a2.deallocate(_index, buckets+1);
	}
}

template <class Allocator>
void json_object_imp<Allocator>::push_back(const json_string &key, const json_null &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_object_imp<Allocator>::push_back(const json_string &key, const json_boolean &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_object_imp<Allocator>::push_back(const json_string &key, const json_number &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_object_imp<Allocator>::push_back(const json_string &key, const json_string &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_object_imp<Allocator>::push_back(const json_string &key, const json_value &value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, value);
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_object_imp<Allocator>::push_back(const json_string &key, const json_var &value) {
	push_back(key, value.value());
}

template <class Allocator>
template <class A2>
void json_object_imp<Allocator>::push_back(const json_string &key, json_array_imp<A2> &&value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, std::move(value));
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
template <class A2>
void json_object_imp<Allocator>::push_back(const json_string &key, json_object_imp<A2> &&value) {
	typename allocator::pointer x = _allocator.allocate(1);
	_allocator.construct(x, key, std::move(value));
	*_tail = x;
	_tail = &x->next;
	++_size;
}

template <class Allocator>
void json_object_imp<Allocator>::index() {
	typename Allocator::template rebind<node *>::other a2(_allocator);
	if(_index) {
		size_t buckets = (size_t)_index[0];
		a2.deallocate(_index, buckets+1);
	}
	size_t buckets = _size / 2;
	buckets = buckets == 0 ? 1 : buckets;
	_index = a2.allocate(buckets+1);
	json_object::index(buckets);
}

inline json_array::const_iterator &json_array::const_iterator::operator++() {
	_pos = _pos->next;
	return *this;
}

inline const json_var &json_array::const_iterator::operator*() const {
	return _pos->value;
}

inline const json_var *json_array::const_iterator::operator->() const {
	return &_pos->value;
}

inline bool json_array::const_iterator::operator!=(const const_iterator &x) const {
	return _pos != x._pos;
}

inline bool json_array::const_iterator::operator==(const const_iterator &x) const {
	return _pos == x._pos;
}

inline json_var &json_array::iterator::operator*() {
	return const_cast<node *>(_pos)->value;
}

inline json_var *json_array::iterator::operator->() {
	return &const_cast<node *>(_pos)->value;
}

inline json_object::const_iterator &json_object::const_iterator::operator++() {
	_pos = _pos->next;
	return *this;
}

inline const std::pair<json_string, json_var> &json_object::const_iterator::operator*() const {
	return _pos->value;
}

inline const std::pair<json_string, json_var> *json_object::const_iterator::operator->() const {
	return &_pos->value;
}

inline bool json_object::const_iterator::operator!=(const const_iterator &x) const {
	return _pos != x._pos;
}

inline bool json_object::const_iterator::operator==(const const_iterator &x) const {
	return _pos == x._pos;
}

inline std::pair<json_string, json_var> &json_object::iterator::operator*() {
	return const_cast<node *>(_pos)->value;
}

inline std::pair<json_string, json_var> *json_object::iterator::operator->() {
	return &const_cast<node *>(_pos)->value;
}

#endif
