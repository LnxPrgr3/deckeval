#include "json.h"
#include <sstream>
#include <functional>
#include <vector>
#include <cmath>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#elif __SSE2__
#include <emmintrin.h>
#endif

const json_var json_none = json_null();

json_value::~json_value() { }

namespace std {
template <>
struct hash<json_string> {
	size_t operator()(const json_string &x) const noexcept {
		static const size_t offset_basis = sizeof(size_t) == 4 ? 2166136261U : 14695981039346656037U;
		static const size_t prime = sizeof(size_t) == 4 ? 16777619U : 1099511628211U;
		size_t hash = offset_basis;
		if(!x._multipart) {
			const char *str = x._value;
			const char *end = str+x._size;
			while(str != end) {
				hash ^= *str++;
				hash *= prime;
			}
		} else {
			json_string::extent *ext = x._next;
			while(ext) {
				const char *str = ext->value();
				const char *end = str+ext->_size;
				while(str != end) {
					hash ^= *str++;
					hash *= prime;
				}
				ext = ext->_next;
			}
		}
		return hash;
	}
};
}

json_null::operator json_boolean() const {
	return json_boolean(false);
}

json_null::operator json_number() const {
	return 0;
}

json_null::operator json_string() const {
	return "";
}

json_null::operator json_array() const {
	throw std::logic_error("Attempt to convert null to JSON array");
}

json_null::operator json_object() const {
	throw std::logic_error("Attempt to convert null to JSON object");
}

json_boolean::operator json_boolean() const {
	return *this;
}

json_boolean::operator json_number() const {
	return _value ? 1 : 0;
}

json_boolean::operator json_string() const {
	return _value ? "true" : "false";
}

json_boolean::operator json_array() const {
	throw std::logic_error("Attempt to convert JSON boolean to array");
}

json_boolean::operator json_object() const {
	throw std::logic_error("Attempt to convert JSON boolean to objecT");
}

json_number::operator json_boolean() const {
	return _value != 0 ? true : false;
}

json_number::operator json_number() const {
	return *this;
}

json_number::operator json_string() const {
	std::stringstream str;
	str << _value;
	const std::string &res = str.str();
	return json_string(res.data(), res.size(), true);
}

json_number::operator json_array() const {
	throw std::logic_error("Attempt to convert JSON number to array");
}

json_number::operator json_object() const {
	throw std::logic_error("Attempt to convert JSON number to object");
}

json_string::operator json_boolean() const {
	return !empty();
}

json_string::operator json_number() const {
	double rv;
	std::stringstream str(_value);
	str >> rv;
	return rv;
}

json_string::operator json_string() const {
	return *this;
}

json_string::operator json_array() const {
	throw std::logic_error("Attempt to convert JSON string to array");
}

json_string::operator json_object() const {
	throw std::logic_error("Attempt to convert JSON string to object");
}

json_array::operator json_boolean() const {
	throw std::logic_error("Attempt to convert JSON array to boolean");
}

json_array::operator json_number() const {
	throw std::logic_error("Attempt to convert JSON array to number");
}

json_array::operator json_string() const {
	std::stringstream str;
	bool wrote = false;
	for(const auto &value: *this) {
		if(wrote)
			str << ',';
		str << json_string(value);
		wrote = true;
	}
	const std::string &res = str.str();
	return json_string(res.data(), res.size(), true);
}

json_array::operator json_array() const {
	return *this;
}

json_array::operator json_object() const {
	throw std::logic_error("Attempt to convert JSON array to object");
}

bool json_object::has_key(const json_string &key) const {
	if(!_index)
		throw std::logic_error("Must index JSON object before doing lookups by key!");
	size_t buckets = (size_t)_index[0];
	size_t bucket = (std::hash<json_string>()(key) % buckets) + 1;
	for(node *value = _index[bucket]; value; value = value->next) {
		if(value->value.first == key)
			return true;
	}
	return false;
}

const json_var &json_object::operator[](const json_string &key) const {
	if(!_index)
		throw std::logic_error("Must index JSON object before doing lookups by key!");
	size_t buckets = (size_t)_index[0];
	size_t bucket = (std::hash<json_string>()(key) % buckets) + 1;
	for(node *value = _index[bucket]; value; value = value->next) {
		if(value->value.first == key)
			return value->value.second;
	}
	return json_none;
}

json_var &json_object::operator[](const json_string &key) {
	if(!_index)
		throw std::logic_error("Must index JSON object before doing lookups by key!");
	size_t buckets = (size_t)_index[0];
	size_t bucket = (std::hash<json_string>()(key) % buckets) + 1;
	for(node *value = _index[bucket]; value; value = value->next) {
		if(value->value.first == key)
			return value->value.second;
	}
	throw std::runtime_error("JSON object key not found");
}

json_object::operator json_boolean() const {
	throw std::logic_error("Attempt to convert JSON object to boolean");
}

json_object::operator json_number() const {
	throw std::logic_error("Attempt to convert JSON object to number");
}

json_object::operator json_string() const {
	throw std::logic_error("Attempt to convert JSON object to string");
}

json_object::operator json_array() const {
	throw std::logic_error("Attempt to convert JSON object to array");
}

json_object::operator json_object() const {
	return *this;
}

void json_object::index(size_t buckets) {
	memset((void *)_index, 0, sizeof(node *)*(buckets+1));
	_index[0] = (node *)buckets;
	std::hash<json_string> hash;
	for(node *value = _head; value != nullptr; value = value->next) {
		value->hash_next = nullptr;
		size_t bucket = (hash(value->value.first) % buckets) + 1;
		if(!_index[bucket])
			_index[bucket] = value;
		else {
			node *hashed = _index[bucket];
			while(hashed->hash_next)
				hashed = hashed->hash_next;
			hashed->hash_next = value;
		}
	}
}

json_var::json_var(const json_value &x) {
	static const std::type_info &nullinfo = typeid(json_null);
	static const std::type_info &boolinfo = typeid(json_boolean);
	static const std::type_info &numinfo = typeid(json_number);
	static const std::type_info &strinfo = typeid(json_string);
	const std::type_info &xinfo = typeid(x);
	if(xinfo == nullinfo) {
		_type = NONE;
		new((void *)&_null) json_null();
	} else if(xinfo == boolinfo) {
		_type = BOOLEAN;
		new((void *)&_boolean) json_boolean(x);
	} else if(xinfo == numinfo) {
		_type = NUMBER;
		new((void *)&_number) json_number(x);
	} else if(xinfo == strinfo) {
		_type = STRING;
		new((void *)&_string) json_string(x);
	} else
		throw std::invalid_argument("json_var::json_var");
}

json_var::json_var(const json_var &x)  {
	*this = x;
}

json_var::~json_var() {
	destroy_subobject();
}

json_var &json_var::operator=(const json_var &x) {
	if(this != &x) {
		destroy_subobject();
		_type = x._type;
		switch(_type) {
		case NONE:
			new((void *)&_null) json_null();
			break;
		case BOOLEAN:
			new((void *)&_boolean) json_boolean(x._boolean);
			break;
		case NUMBER:
			new((void *)&_number) json_number(x._number);
			break;
		case STRING:
			new((void *)&_string) json_string(x._string);
			break;
		case ARRAY:
			new((void *)&_array) json_array(x._array);
			break;
		case OBJECT:
			new((void *)&_object) json_object(x._object);
			break;
		}
	}
	return *this;
}

void json_var::destroy_subobject() {
	switch(_type) {
	case NONE:
		_null.~json_null();
		break;
	case BOOLEAN:
		_boolean.~json_boolean();
		break;
	case NUMBER:
		_number.~json_number();
		break;
	case STRING:
		_string.~json_string();
		break;
	case ARRAY:
		_array.~json_array();
		break;
	case OBJECT:
		_object.~json_object();
		break;
	}
}

const json_value &json_var::value() const {
	switch(_type) {
	case NONE:
		return _null;
	case BOOLEAN:
		return _boolean;
	case NUMBER:
		return _number;
	case STRING:
		return _string;
	case ARRAY:
		return _array;
	case OBJECT:
		return _object;
	}
}

json_var::operator json_boolean() const {
	return value();
}

json_var::operator json_number() const {
	return value();
}

json_var::operator json_string() const {
	return value();
}

json_var::operator json_array() const {
	return value();
}

json_var::operator json_object() const {
	return value();
}

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

void json_document::shrink_to_fit() {
	const auto page_size = mapping::page_size();
	size_t size = (_heap.pos / page_size) * page_size + (_heap.pos % page_size ? page_size : 0);
	_heap.data.truncate(size);
}

struct false_cond {
	bool operator()() const { return false; }
};

#ifdef __ARM_NEON__
template <class CharOp, class VecOp, class Cond = false_cond>
const char *neon_scan(const char *data, const char *end, CharOp &&cop, VecOp &&vop, Cond &&cond = Cond()) {
	if(data < end-15) {
		vop(vld1q_u8((uint8_t *)data));
		if(cond()) return data;
		const char *nd __attribute__((__aligned__(16))) = (const char *)(((uintptr_t)data / 16 + 1) * 16);
		while(nd < end) {
			vop(vld1q_u8((uint8_t *)nd));
			if(cond()) return nd;
			nd += 16;
		}
		data = nd;
	}
	while(data < end) {
		cop(*data); if(cond()) return data; ++data;
	}
	return data > end ? end : data;
}

const char *neon_memchr(const char *data, const char *end, char needle, char needle2) {
	auto needle_expanded = vdupq_n_u8(needle);
	auto needle2_expanded = vdupq_n_u8(needle2);
	uint8x16_t index = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	bool done = false;
	size_t offset = 0;
	data = neon_scan(data, end, [&done,needle,needle2](char c) {
		if(c == needle || c == needle2)
			done = true;
	}, [&done,&offset,needle_expanded,needle2_expanded,index](uint8x16_t haystack) {
		auto eq = vceqq_u8(haystack, needle_expanded); // 0xff if found
		eq = vorrq_u8(eq, vceqq_u8(haystack, needle2_expanded));
		eq = vmvnq_u8(eq); // 0 if found
		auto res = vorrq_u8(eq, index); // Index or 0xff
		auto res_high = vget_high_u8(res);
		auto res_low = vget_low_u8(res);
		auto res_min = vmin_u8(res_high, res_low); // 8 possible indexes
		auto res_shifted = vext_u8(res_min, res_min, 4);
		auto res_intsize_min = vmin_u8(res_min, res_shifted);
		auto res_intsize_shifted = vext_u8(res_intsize_min, res_intsize_min, 2);
		auto res_shortsize_min = vmin_u8(res_intsize_min, res_intsize_shifted);
		auto res_shortsize_shifted = vext_u8(res_shortsize_min, res_shortsize_min, 1);
		auto res_charsize_min = vmin_u8(res_shortsize_min, res_shortsize_shifted);
		auto res_char_min = vget_lane_u8(res_charsize_min, 0);
		if(res_char_min != 0xff) {
			offset = res_char_min;
			done = true;
		}
	}, [&done]() { return done; });
	return data + offset;
}
#elif __SSE2__
template <class CharOp, class VecOp, class Cond = false_cond>
const char *sse2_scan(const char *data, const char *end, CharOp &&cop, VecOp &&vop, Cond &&cond = Cond()) {
	if(data < end-15) {
		vop(_mm_loadu_si128((const __m128i *)data));
		if(cond()) return data;
		const char *nd __attribute__((__aligned__(16))) = (const char *)(((uintptr_t)data / 16 + 1) * 16);
		while(nd < end) {
			vop(_mm_load_si128((const __m128i *)nd));
			if(cond()) return nd;
			nd += 16;
		}
		data = nd;
	}
	while(data < end) {
		cop(*data); if(cond()) return data; ++data;
	}
	return data > end ? end : data;
}

const char *sse2_memchr(const char *data, const char *end, char needle, char needle2) {
	auto needle_expanded = _mm_set1_epi8(needle);
	auto needle2_expanded = _mm_set1_epi8(needle2);
	bool done = false;
	size_t offset = 0;
	data = sse2_scan(data, end, [&done,needle,needle2](char c) {
		if(c == needle || c == needle2)
			done = true;
	}, [&done,&offset,needle_expanded,needle2_expanded](__m128i haystack) {
		auto eq = _mm_cmpeq_epi8(haystack, needle_expanded);
		eq = _mm_or_si128(eq, _mm_cmpeq_epi8(haystack, needle2_expanded));
		auto mask = _mm_movemask_epi8(eq);
		if(mask) {
			offset = __builtin_ffs(mask)-1;
			done = true;
		}
	}, [&done]() { return done; });
	return data + offset;
}
#endif

const char *json_skip_whitespace(const char *begin, const char *end) {
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

void json_nonempty(const char *begin, const char *end) {
	if(begin == end)
		throw json_exception("Unexpected end of input");
}

const char *json_parse_char(const char *begin, const char *end, char c) {
	if(begin != end && *begin == c)
		return ++begin;
	throw json_exception("");
}

const char *json_parse_boolean(const char *begin, const char *end, json_callbacks &cb) {
	try {
		switch(*begin) {
		case 't':
			++begin;
			begin = json_parse_char(begin, end, 'r');
			begin = json_parse_char(begin, end, 'u');
			begin = json_parse_char(begin, end, 'e');
			begin = json_skip_whitespace(begin, end);
			cb.boolean(true);
			break;
		case 'f':
			++begin;
			begin = json_parse_char(begin, end, 'a');
			begin = json_parse_char(begin, end, 'l');
			begin = json_parse_char(begin, end, 's');
			begin = json_parse_char(begin, end, 'e');
			cb.boolean(false);
			break;
		}
		return begin;
	} catch (json_exception &e) {
		throw json_exception("Unexpected bare word");
	}
}

const char *json_parse_number(const char *begin, const char *end, json_callbacks &cb) {
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
	cb.number(sign*(number+frac*pow(0.1, fracdigits))*pow(10, expsign*exp));
	return begin;
}

char *json_str_write(char *pos, char c) {
	*pos = c;
	return ++pos;
}

template <class Allocator>
void json_str_write_codepoint(json_string_imp<Allocator> &str, unsigned int codepoint) {
	if(codepoint < 0b10000000) {
		*str.append_internal(1) = codepoint;
	} else {
		int len = codepoint < 0x800 ? 2 :
			      codepoint < 0x10000 ? 3 :
		          4;
		char *pos = str.append_internal(len);
		codepoint = codepoint << (32 - 5 * len - 1);
		pos = json_str_write(pos,  (0xffu << (8 - len)) | ((codepoint & (0xffu << (32 - (7 - len)))) >> (24 + len + 1)));
		codepoint <<= 7 - len;
		switch(len) {
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
}

int json_parse_hex(const char *begin, const char *end) {
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

template <class Allocator>
const char *json_parse_string_slow(json_string_imp<Allocator> &&str, const char *begin, const char *end, json_callbacks &cb) {
	while(begin != end && *begin != '"') {
		if(*begin == '\\') {
			unsigned int codepoint = 0;
			json_nonempty(++begin, end);
			switch(*begin) {
			case '"':
			case '\\':
			case '/':
				*str.append_internal(1) = *begin;
				break;
			case 'b':
				*str.append_internal(1) = '\b';
				break;
			case 'f':
				*str.append_internal(1) = '\f';
				break;
			case 'n':
				*str.append_internal(1) = '\n';
				break;
			case 'r':
				*str.append_internal(1) = '\r';
				break;
			case 't':
				*str.append_internal(1) = '\t';
				break;
			case 'u':
				codepoint = json_parse_hex(++begin, end);
				codepoint = codepoint * 16 + json_parse_hex(++begin, end);
				codepoint = codepoint * 16 + json_parse_hex(++begin, end);
				codepoint = codepoint * 16 + json_parse_hex(++begin, end);
				json_str_write_codepoint(str, codepoint);
				break;
			default:
				throw json_exception("Unrecognized string escape sequence");
			}
		} else
			*str.append_internal(1) = *begin;
		++begin;
#ifdef __ARM_NEON__
		const char *next = neon_memchr(begin, end, '\\', '"');
		size_t size = next-begin;
		if(size) {
			str.append(json_string(begin, size));
			begin = next;
		}
#elif __SSE2__
		const char *next = sse2_memchr(begin, end, '\\', '"');
		size_t size = next-begin;
		if(size) {
			str.append(json_string(begin, size));
			begin = next;
		}
#else
	const char *next = begin;
	while(next != end && *next != '"' && *next != '\\') ++next;
	size_t size = next-begin;
	if(size) {
		str.append(json_string(begin, size));
		begin = next;
	}
#endif
	}
	json_nonempty(begin, end);
	cb.string(std::move(str));
	return ++begin;
}

template <class Allocator>
json_string_imp<typename Allocator::template rebind<json_string::extent>::other> make_string_imp(Allocator &allocator, const char *begin, const char *end) {
	typename Allocator::template rebind<json_string::extent>::other a2(allocator);
	return json_string_imp<typename Allocator::template rebind<json_string::extent>::other>(begin, end-begin, a2);
}

template <class Allocator>
__attribute__((always_inline)) inline const char *json_parse_string(Allocator &allocator, const char *begin, const char *end, json_callbacks &cb) {
	const char *str = begin;
#ifdef __ARM_NEON__
	begin = neon_memchr(begin, end, '\\', '"');
	if(*begin == '\\')
		return json_parse_string_slow(make_string_imp(allocator, str, begin), begin, end, cb);
	json_nonempty(begin, end);
	cb.string(json_string(str, begin-str));
	return ++begin;
#elif __SSE2__
	begin = sse2_memchr(begin, end, '\\', '"');
	if(*begin == '\\')
		return json_parse_string_slow(make_string_imp(allocator, str, begin), begin, end, cb);
	json_nonempty(begin, end);
	cb.string(json_string(str, begin-str));
	return ++begin;
#else
	while(begin != end && *begin != '"') {
		if(*begin == '\\')
			return json_parse_string_slow(make_string_imp(allocator, str, begin), begin, end, cb);
		++begin;
	}
	json_nonempty(begin, end);
	cb.string(json_string(str, begin-str));
	return ++begin;
#endif
}

template <class Allocator>
const char *json_parse_array(Allocator &allocator, const char *begin, const char *end, json_callbacks &cb) {
	cb.array_begin();
	begin = json_skip_whitespace(begin, end);
	json_nonempty(begin, end);
	while(*begin != ']') {
		begin = json_parse(allocator, begin, end, cb);
		begin = json_skip_whitespace(begin, end);
		json_nonempty(begin, end);
		if(*begin == ',') {
			begin = json_skip_whitespace(++begin, end);
			json_nonempty(begin, end);
		} else if(*begin != ']')
			throw json_exception("Array members must be separated by commas");
	}
	cb.array_end();
	return ++begin;
}

template <class Allocator>
const char *json_parse_object(Allocator &allocator, const char *begin, const char *end, json_callbacks &cb) {
	cb.object_begin();
	begin = json_skip_whitespace(begin, end);
	json_nonempty(begin, end);
	while(*begin != '}') {
		if(*begin != '"')
			throw json_exception("Invalid object key");
		begin = json_parse_string(allocator, ++begin, end, cb);
		begin = json_skip_whitespace(begin, end);
		json_nonempty(begin, end);
		begin = json_parse_char(begin, end, ':');
		begin = json_skip_whitespace(begin, end);
		begin = json_parse(allocator, begin, end, cb);
		begin = json_skip_whitespace(begin, end);
		json_nonempty(begin, end);
		if(*begin == ',') {
			begin = json_skip_whitespace(++begin, end);
			json_nonempty(begin, end);
		} else if(*begin != '}')
			throw json_exception("Object key/value pairs must be separated by commas");
	}
	cb.object_end();
	return ++begin;
}

template <class Allocator>
const char *json_parse(Allocator &allocator, const char *begin, const char *end, json_callbacks &cb) {
	begin = json_skip_whitespace(begin, end);
	json_nonempty(begin, end);
	switch(*begin) {
	case 'n':
		++begin;
		begin = json_parse_char(begin, end, 'u');
		begin = json_parse_char(begin, end, 'l');
		begin = json_parse_char(begin, end, 'l');
		cb.null();
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
		begin = json_parse_string(allocator, ++begin, end, cb);
		break;
	case '{':
		begin = json_parse_object(allocator, ++begin, end, cb);
		break;
	case '[':
		begin = json_parse_array(allocator, ++begin, end, cb);
		break;
	}
	return begin;
}

const char *json_parse(const char *begin, const char *end, json_callbacks &cb) {
	std::allocator<void> allocator;
	return json_parse(allocator, begin, end, cb);
}

struct json_parse_callbacks : public json_callbacks {
	typedef json_array_imp<json_allocator<json_var>> json_arr;
	typedef json_object_imp<json_allocator<std::pair<json_string, json_var>>> json_obj;

	struct object {
		const enum {NONE, ARRAY, OBJECT} type;
		union {
			json_arr arr;
			json_obj obj;
		};
		json_string key;
		object() : type(NONE), key(nullptr, 0) { }
		object(decltype(type) type, const json_allocator<char> &allocator) : type(type), key(nullptr, 0) {
			switch(type) {
			case NONE:
				break;
			case ARRAY:
				new (&arr) json_arr(allocator);
				break;
			case OBJECT:
				new (&obj) json_obj(allocator);
				break;
			}
		}
		object(object &&x) : type(x.type), key(x.key) {
			switch(type) {
			case NONE:
				break;
			case ARRAY:
				new (&arr) json_arr(std::move(x.arr));
				break;
			case OBJECT:
				new (&obj) json_obj(std::move(x.obj));
				break;
			}
		}
		~object() {
			switch(type) {
			case NONE:
				break;
			case ARRAY:
				arr.~json_arr();
				break;
			case OBJECT:
				obj.~json_obj();
				break;
			}
		}
	};

	json_parse_callbacks(size_t preallocate) : rv(preallocate) { }
	template <class Value>
	void add(object *next, Value &&value) {
		if(next) {
			switch(next->type) {
			case object::NONE:
				break;
			case object::ARRAY:
				next->arr.push_back(std::move(value));
				break;
			case object::OBJECT:
				if(next->key.is_null()) {
					next->key = value;
				} else {
					next->obj.push_back(next->key, std::move(value));
					next->key = json_string(nullptr, 0);
				}
				break;
			}
		} else {
			rv = std::move(value);
		}
	}
	object *next(size_t n = 1) {
		size_t depth = stack.size();
		if(depth >= n)
			return &stack[depth-n];
		return nullptr;
	}
	void null() {
		add(next(), json_null());
	}
	void boolean(const json_boolean &value) {
		add(next(), value);
	}
	void number(const json_number &value) {
		add(next(), value);
	}
	void string(const json_string &value) {
		add(next(), value);
	}
	void array_begin() {
		stack.emplace_back(object::ARRAY, rv._allocator);
	}
	void array_end() {
		add(next(2), std::move(stack.back().arr));
		stack.pop_back();
	}
	void object_begin() {
		stack.emplace_back(object::OBJECT, rv._allocator);
	}
	void object_end() {
		add(next(2), std::move(stack.back().obj));
		stack.pop_back();
	}

	json_document rv;
	std::vector<object> stack;
};

json_document json_parse(const char *begin, const char *end) {
	json_parse_callbacks cb((end-begin)*sizeof(void *));
	json_parse(cb.rv._allocator, begin, end, cb);
	cb.rv.shrink_to_fit();
	return std::move(cb.rv);
}
