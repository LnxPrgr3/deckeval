#include "file.h"
#include "json.h"
#include "carddb.h"

void card_database::cost::parse_error() {
	throw std::runtime_error("Invalid mana cost");
}

void card_database::cost::parse_next(json_string::const_iterator str, json_string::const_iterator end) {
	_exists = 1;
	if(str == end)
		parse_error();
	if(*str++ != '}')
		parse_error();
	return parse(str, end);
}

void card_database::cost::parse(json_string::const_iterator str, json_string::const_iterator end) {
	if(str == end)
		return;
	if(*str++ != '{')
		parse_error();
	if(str == end)
		parse_error();
	switch(*str) {
	case 'W':
		++str;
		if(str != end && *str == '/') {
			++str;
			if(str == end)
				parse_error();
			switch(*str++) {
			case 'U':
				++_whiteblue;
				break;
			case 'B':
				++_whiteblack;
				break;
			case 'R':
				++_whitered;
				break;
			case 'G':
				++_whitegreen;
				break;
			case 'P':
				++_whitephyrexian;
				break;
			default:
				parse_error();
			}
		} else
			++_white;
		return parse_next(str, end);
	case 'U':
		++str;
		if(str != end && *str == '/') {
			++str;
			if(str == end)
				parse_error();
			switch(*str++) {
			case 'W':
				++_whiteblue;
				break;
			case 'B':
				++_blueblack;
				break;
			case 'R':
				++_bluered;
				break;
			case 'G':
				++_bluegreen;
				break;
			case 'P':
				++_bluephyrexian;
				break;
			default:
				parse_error();
			}
		} else
			++_blue;
		return parse_next(str, end);
	case 'B':
		++str;
		if(str != end && *str == '/') {
			++str;
			if(str == end)
				parse_error();
			switch(*str++) {
			case 'W':
				++_whiteblack;
				break;
			case 'U':
				++_bluered;
				break;
			case 'R':
				++_blackred;
				break;
			case 'G':
				++_blackgreen;
				break;
			case 'P':
				++_blackphyrexian;
				break;
			default:
				parse_error();
			}
		} else
			++_black;
		return parse_next(str, end);
	case 'R':
		++str;
		if(str != end && *str == '/') {
			++str;
			if(str == end)
				parse_error();
			switch(*str++) {
			case 'W':
				++_whitered;
				break;
			case 'U':
				++_bluered;
				break;
			case 'B':
				++_blackred;
				break;
			case 'G':
				++_redgreen;
				break;
			case 'P':
				++_redphyrexian;
				break;
			default:
				parse_error();
			}
		} else
			++_red;
		return parse_next(str, end);
	case 'G':
		++str;
		if(str != end && *str == '/') {
			++str;
			if(str == end)
				parse_error();
			switch(*str++) {
			case 'W':
				++_whitegreen;
				break;
			case 'U':
				++_bluegreen;
				break;
			case 'B':
				++_blackgreen;
				break;
			case 'R':
				++_redgreen;
				break;
			case 'P':
				++_redphyrexian;
				break;
			default:
				parse_error();
			}
		} else
			++_green;
		return parse_next(str, end);
	case 'C':
		++_colorless;
		return parse_next(++str, end);
	case 'X':
		++_x;
		return parse_next(++str, end);
	case 'Y':
		++_y;
		return parse_next(++str, end);
	case 'Z':
		++_z;
		return parse_next(++str, end);
	case 'T':
		_tap = 1;
		return parse_next(++str, end);
	case 'q':
		_untap = 1;
		return parse_next(++str, end);
	case 'h':
		++str;
		if(str == end)
			parse_error();
		switch(*str++) {
		case 'w':
			_halfwhite = 1;
			break;
		case 'u':
			_halfblue = 1;
			break;
		case 'b':
			_halfblack = 1;
			break;
		case 'r':
			_halfred = 1;
			break;
		case 'g':
			_halfgreen = 1;
			break;
		default:
			parse_error();
		}
		return parse_next(str, end);
	case '0':
	case '1':
	case '2': {
		auto str2 = str; ++str2;
		if(str2 != end && *str2 == '/') {
			++str2;
			if(str == end)
				parse_error();
			switch(*str2++) {
				case 'W':
					++_2white;
					break;
				case 'U':
					++_2blue;
					break;
				case 'B':
					++_2black;
					break;
				case 'R':
					++_2red;
					break;
				case 'G':
					++_2green;
					break;
			}
			return parse_next(str2, end);
		}
	}
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if(_generic != 0)
			parse_error();
		while(str != end && *str >= '0' && *str <= '9') {
			_generic = _generic*10 + (*str++ - '0');
		}
		return parse_next(str, end);
	default:
		parse_error();
	}
}

void card_database::deck::init() {
	json_document doc = json_parse(&*_str.begin(), &*_str.end());
	json_object val = doc;
	_name = val["name"];
	for(const json_object &card: json_array(val["deck"])) {
		_deck.emplace_back(_parent.find_card(card["name"]), (int)json_number(card["count"]));
	}
	for(const json_object &card: json_array(val["sideboard"])) {
		_sideboard.emplace_back(_parent.find_card(card["name"]), (int)json_number(card["count"]));
	}
}

card_database::card_database(const char *filename) : _mapping(load(filename)), _sets(parse(_mapping)) {
	size_t cards = 0;
	for(auto set: sets()) {
		cards += set.cards().size();
	}
	_cards.reserve(cards);
	for(auto set: sets()) {
		for(auto card: set.cards()) {
			_cards.emplace(card.name(), card);
		}
	}
}

mapping card_database::load(const char *filename) {
	return mapping::options()
		.file(file::options(filename).open())
		.map();
}

std::ostream &operator<<(std::ostream &out, const card_database::cost &x) {
	if(!x.exists())
		return out;
	bool wrote = false;
	for(int i = 0; i < x.x(); ++i) {
		out << "{X}";
		wrote = true;
	}
	for(int i = 0; i < x.y(); ++i) {
		out << "{Y}";
		wrote = true;
	}
	for(int i = 0; i < x.z(); ++i) {
		out << "{Z}";
		wrote = true;
	}
	if(x.generic()) {
		out << '{' << x.generic() << '}';
		wrote = true;
	}
	for(int i = 0; i < x.whiteblue(); ++i) {
		out << "{W/U}";
		wrote = true;
	}
	for(int i = 0; i < x.whiteblack(); ++i) {
		out << "{W/B}";
		wrote = true;
	}
	for(int i = 0; i < x.whitered(); ++i) {
		out << "{W/R}";
		wrote = true;
	}
	for(int i = 0; i < x.whitegreen(); ++i) {
		out << "{W/G}";
		wrote = true;
	}
	for(int i = 0; i < x.whitephyrexian(); ++i) {
		out << "{W/P}";
		wrote = true;
	}
	for(int i = 0; i < x.twowhite(); ++i) {
		out << "{2/W}";
		wrote = true;
	}
	if(x.halfwhite()) {
		out << "{hw}";
		wrote = true;
	}
	for(int i = 0; i < x.white(); ++i) {
		out << "{W}";
		wrote = true;
	}
	for(int i = 0; i < x.blueblack(); ++i) {
		out << "{U/B}";
		wrote = true;
	}
	for(int i = 0; i < x.bluered(); ++i) {
		out << "{U/R}";
		wrote = true;
	}
	for(int i = 0; i < x.bluegreen(); ++i) {
		out << "{U/G}";
		wrote = true;
	}
	for(int i = 0; i < x.bluephyrexian(); ++i) {
		out << "{U/P}";
		wrote = true;
	}
	for(int i = 0; i < x.twoblue(); ++i) {
		out << "{2/U}";
		wrote = true;
	}
	if(x.halfblue()) {
		out << "{hu}";
		wrote = true;
	}
	for(int i = 0; i < x.blue(); ++i) {
		out << "{U}";
		wrote = true;
	}
	for(int i = 0; i < x.blackred(); ++i) {
		out << "{B/R}";
		wrote = true;
	}
	for(int i = 0; i < x.blackgreen(); ++i) {
		out << "{B/G}";
		wrote = true;
	}
	for(int i = 0; i < x.blackphyrexian(); ++i) {
		out << "{B/P}";
		wrote = true;
	}
	for(int i = 0; i < x.twoblack(); ++i) {
		out << "{2/B}";
		wrote = true;
	}
	if(x.halfblack()) {
		out << "{hb}";
		wrote = true;
	}
	for(int i = 0; i < x.black(); ++i) {
		out << "{B}";
		wrote = true;
	}
	for(int i = 0; i < x.redgreen(); ++i) {
		out << "{R/G}";
		wrote = true;
	}
	for(int i = 0; i < x.redphyrexian(); ++i) {
		out << "{R/P}";
		wrote = true;
	}
	for(int i = 0; i < x.twored(); ++i) {
		out << "{2/R}";
		wrote = true;
	}
	if(x.halfred()) {
		out << "{hr}";
		wrote = true;
	}
	for(int i = 0; i < x.red(); ++i) {
		out << "{R}";
		wrote = true;
	}
	for(int i = 0; i < x.greenphyrexian(); ++i) {
		out << "{G/P}";
		wrote = true;
	}
	for(int i = 0; i < x.twogreen(); ++i) {
		out << "{2/G}";
		wrote = true;
	}
	if(x.halfgreen()) {
		out << "{hg}";
		wrote = true;
	}
	for(int i = 0; i < x.green(); ++i) {
		out << "{G}";
		wrote = true;
	}
	for(int i = 0; i < x.colorless(); ++i) {
		out << "{C}";
		wrote = true;
	}
	if(x.tap()) {
		out << "{T}";
		wrote = true;
	}
	if(x.untap()) {
		out << "{q}";
		wrote = true;
	}
	if(!wrote)
		out << "{0}";
	return out;
}

