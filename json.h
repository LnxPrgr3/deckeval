#ifndef DECKEVAL_JSON_H
#define DECKEVAL_JSON_H
#include <exception>
#include <cmath>

class json_exception : public std::exception {
public:
	json_exception(const char *msg) : _what(msg) {}
	const char *what() const noexcept { return _what; }
private:
	const char * const _what;
};

class json_string {
public:
	json_string(const char *data, size_t len) : _data(data), _len(len) {}
	const char *c_str() const { return _data; }
	size_t size() const { return _len; }
private:
	const char *_data;
	size_t _len;
};

class json_object {};
class json_array {};
class json_end {};

class json_value {
public:
	json_value() : _type(NONE) {}
	json_value(bool value) : _type(BOOLEAN), _boolean(value) {}
	json_value(double value) : _type(NUMBER), _number(value) {}
	json_value(const char *start, const char *end) : _type(STRING), _string(start, end-start) {}
	json_value(const json_object &) : _type(OBJECT) {}
	json_value(const json_array &) : _type(ARRAY) {}
	json_value(const json_end &) : _type(END) {}
	bool is_null() const { return _type == NONE; }
	bool is_boolean() const { return _type == BOOLEAN; }
	bool as_boolean() {
		if(!is_boolean())
			throw json_exception("Invalid conversion to boolean");
		return _boolean;
	}
	bool is_number() const { return _type == NUMBER; }
	double as_number() const {
		if(!is_number())
			throw json_exception("Invalid conversion to double");
		return _number;
	}
	bool is_string() const { return _type == STRING; }
	json_string as_string() const {
		if(!is_string())
			throw json_exception("Invalid conversion to string");
		return _string;
	}
	bool is_object() const { return _type == OBJECT; }
	bool is_array() const { return _type == ARRAY; }
	bool is_end() const { return _type == END; }
private:
	enum {NONE, BOOLEAN, NUMBER, STRING, OBJECT, ARRAY, END} _type;
	union {
		bool _boolean;
		double _number;
		json_string _string;
	};
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

#endif
