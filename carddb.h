#ifndef DECKEVAL_CARDDB_H
#define DECKEVAL_CARDDB_H
#include "mapping.h"
#include "json.h"
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace std {
template <>
struct hash<json_string> {
	size_t operator()(const json_string &x) const noexcept {
		static const size_t offset_basis = sizeof(size_t) == 4 ? 2166136261U : 14695981039346656037U;
		static const size_t prime = sizeof(size_t) == 4 ? 16777619U : 1099511628211U;
		const char *str = x.c_str();
		const char *end = str+x.size();
		size_t hash = offset_basis;
		while(str != end) {
			hash ^= *str++;
			hash *= prime;
		}
		return hash;
	}
};
}

class card_database {
public:
	typedef json_value_imp<json_allocator<char>> json_value;

	template <class Value>
	class object_collection {
	public:
		class iterator {
		public:
			Value operator*() const { return Value(_iterator->second); }
			iterator &operator++() { ++_iterator; return *this; }
			bool operator!=(const iterator &x) { return _iterator != x._iterator; }
			friend class object_collection;
		private:
			typedef decltype(json_value().as_object().begin()) native_iterator;
			iterator(const native_iterator &x) : _iterator(x) { }
			native_iterator _iterator;
		};

		iterator begin() const { return _collection.begin(); }
		iterator end() const { return _collection.end(); }

		Value operator[](const char *key) {
			return Value(_collection[key]);
		}

		friend class card_database;
	private:
		object_collection(const json_value &x) : _collection(x.as_object()) { }

		const decltype(json_value().as_object()) _collection;
	};

	template <class Value>
	class array_collection {
	public:
		class iterator {
		public:
			Value operator*() const { return Value(*_iterator); }
			iterator &operator++() { ++_iterator; return *this; }
			bool operator!=(const iterator &x) { return _iterator != x._iterator; }
			friend class array_collection;
		private:
			typedef decltype(json_value().as_array().begin()) native_iterator;
			iterator(const native_iterator &x) : _iterator(x) { }
			native_iterator _iterator;
		};

		iterator begin() const { return _collection.begin(); }
		iterator end() const { return _collection.end(); }
		size_t size() const { return _collection.size(); }

		friend class card_database;
	private:
		array_collection(const json_value &x) : _collection(x.as_array()) { }

		decltype(json_value().as_array()) _collection;
	};

	class cost {
	public:
		cost() {
			memset(this, 0, sizeof(*this));
		}

		cost(const json_string &x) : cost() {
			const char *str = x.c_str();
			const char *end = str+x.size();
			try {
				parse(str, end);
			} catch (const std::exception &e) {
				throw std::runtime_error(std::string("Invalid mana cost: ") + std::string(str, x.size()));
			}
		}
		int white() const { return _white; }
		int halfwhite() const { return _halfwhite; }
		int twowhite() const { return _2white; }
		int whiteblue() const { return _whiteblue; }
		int whiteblack() const { return _whiteblack; }
		int whitered() const { return _whitered; }
		int whitegreen() const { return _whitegreen; }
		int whitephyrexian() const { return _whitephyrexian; }
		int blue() const { return _blue; }
		int halfblue() const { return _halfblue; }
		int twoblue() const { return _2blue; }
		int blueblack() const { return _blueblack; }
		int bluered() const { return _bluered; }
		int bluegreen() const { return _bluegreen; }
		int bluephyrexian() const { return _bluephyrexian; }
		int black() const { return _black; }
		int halfblack() const { return _halfblack; }
		int twoblack() const { return _2black; }
		int blackred() const { return _blackred; }
		int blackgreen() const { return _blackgreen; }
		int blackphyrexian() const { return _blackphyrexian; }
		int red() const { return _red; }
		int halfred() const { return _halfred; }
		int twored() const { return _2red; }
		int redgreen() const { return _redgreen; }
		int redphyrexian() const { return _redphyrexian; }
		int green() const { return _green; }
		int halfgreen() const { return _halfgreen; }
		int twogreen() const { return _2green; }
		int greenphyrexian() const { return _greenphyrexian; }
		int colorless() const { return _colorless; }
		int x() const { return _x; }
		int y() const { return _y; }
		int z() const { return _z; }
		bool exists() const { return _exists; }
		bool tap() const { return _tap; }
		bool untap() const { return _untap; }
		int generic() const { return _generic; }
	private:
		void parse_error();
		void parse_next(const char *str, const char *end);
		void parse(const char *str, const char *end);

		unsigned int _white:6,
		             _2white:6,
		             _whiteblue:6,
		             _whiteblack:6,
		             _whitered:6,
		             _whitegreen:6,
		             _whitephyrexian:6,
		             _blue:6,
		             _2blue:6,
		             _bluewhite:6,
		             _blueblack:6,
		             _bluered:6,
		             _bluegreen:6,
		             _bluephyrexian:6,
		             _black:6,
		             _2black:6,
		             _blackred:6,
		             _blackgreen:6,
		             _blackphyrexian:6,
		             _red:6,
		             _2red:6,
		             _redgreen:6,
		             _redphyrexian:6,
		             _green:6,
		             _2green:6,
		             _greenphyrexian:6,
		             _colorless:6,
		             _x:2,
		             _y:2,
		             _z:2,
		             _halfwhite:1,
		             _halfblue:1,
		             _halfblack:1,
		             _halfred:1,
		             _halfgreen:1,
		             _exists:1,
		             _tap:1,
		             _untap:1;
		unsigned int _generic;
	};

	class card {
	public:
		json_string id() const { return _card["id"].as_string(); }
		json_string layout() const { return _card["layout"].as_string(); }
		json_string name() const { return _card["name"].as_string(); }
		decltype(json_value().as_array()) names() const { return _card["names"].as_array(); }
		cost mana_cost() const {
			return cost(_card.has_key("manaCost") ? _card["manaCost"].as_string() : json_string("", 0));
		}
		int cmc() const { return _card.has_key("cmc") ? (int)_card["cmc"].as_number() : 0; }
		decltype(json_value().as_array()) colors() const { return _card["colors"].as_array(); }
		decltype(json_value().as_array()) color_identity() const { return _card["colorIdentity"].as_array(); }
		json_string type() const { return _card["type"].as_string(); }
		decltype(json_value().as_array()) supertypes() const { return _card["supertypes"].as_array(); }
		decltype(json_value().as_array()) types() const { return _card["types"].as_array(); }
		decltype(json_value().as_array()) subtypes() const { return _card["subtypes"].as_array(); }
		json_string rarity() const { return _card["rarity"].as_string(); }
		json_string text() const { return _card.has_key("text") ? _card["text"].as_string() : json_string("", 0); }
		json_string flavor() const { return _card["flavor"].as_string(); }
		json_string artist() const { return _card["artist"].as_string(); }
		json_string number() const { return _card["number"].as_string(); }
		json_string power() const { return _card.has_key("power") ? _card["power"].as_string() : json_string("", 0); }
		json_string toughness() const { return _card.has_key("toughness") ? _card["toughness"].as_string() : json_string("", 0); }
		int loyalty() const { return _card.has_key("loyalty") ? (int)_card["loyalty"].as_number() : 0; }
		int multiverse_id() const { return _card.has_key("multiverseid") ? (int)_card["multiverseid"].as_number() : 0; }

		friend class array_collection<card>;
	private:
		card(const json_value &x) : _card(x) { }

		const json_value &_card;
	};

	class card_set {
	public:
		json_string name() const { return _set["name"].as_string(); }
		json_string code() const { return _set["code"].as_string(); }
		json_string gatherer_code() const {
			return _set.has_key("gathererCode") ? _set["gathererCode"].as_string() : _set["code"].as_string();
		}
		json_string release_date() const { return _set["releaseDate"].as_string(); }
		json_string border() const { return _set["border"].as_string(); }
		json_string type() const { return _set.has_key("type") ?  _set["type"].as_string() : json_string("", 0); }
		json_string block() const { return _set["block"].as_string(); }
		bool online_only() const {
			return _set.has_key("onlineOnly") ? _set["onlineOnly"].as_boolean() : false;
		}
		array_collection<card> cards() const { return array_collection<card>(_set["cards"]); }
		friend class card_database;
		friend class object_collection<card_set>;
	private:
		card_set(const json_value &x) : _set(x) { }

		const json_value &_set;
	};

	card_database(const char *filename);
	object_collection<card_set> sets() const { return object_collection<card_set>(_sets); }
	card find_card(const char *name) {
		json_string str(name, strlen(name));
		auto res = _cards.find(str);
		if(res == _cards.end())
			throw std::runtime_error("Card not found!");
		return res->second;
	}
private:
	static mapping load(const char *filename);
	static json_document parse(mapping &data) {
		auto str = (char *)data.data();
		return json_parse(str, str+data.size());
	}
	mapping _mapping;
	json_document _sets;
	std::unordered_map<json_string, card> _cards;
};

std::ostream &operator<<(std::ostream &out, const card_database::cost &x);

#endif
