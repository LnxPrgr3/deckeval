#ifndef DECKEVAL_CARDDB_H
#define DECKEVAL_CARDDB_H
#include "file.h"
#include "mapping.h"
#include "json.h"

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

		friend class card_database;
	private:
		array_collection(const json_value &x) : _collection(x.as_array()) { }

		decltype(json_value().as_array()) _collection;
	};

	class card {
	public:
		json_string id() const { return _card["id"].as_string(); }
		json_string layout() const { return _card["layout"].as_string(); }
		json_string name() const { return _card["name"].as_string(); }
		decltype(json_value().as_array()) names() const { return _card["names"].as_array(); }
		json_string mana_cost() const { return _card["manaCost"].as_string(); }
		int cmc() const { return _card.has_key("cmc") ? (int)_card["cmc"].as_number() : 0; }
		decltype(json_value().as_array()) colors() const { return _card["colors"].as_array(); }
		decltype(json_value().as_array()) color_identity() const { return _card["colorIdentity"].as_array(); }
		json_string type() const { return _card["type"].as_string(); }
		decltype(json_value().as_array()) supertypes() const { return _card["supertypes"].as_array(); }
		decltype(json_value().as_array()) types() const { return _card["types"].as_array(); }
		decltype(json_value().as_array()) subtypes() const { return _card["subtypes"].as_array(); }
		json_string rarity() const { return _card["rarity"].as_string(); }
		json_string text() const { return _card["text"].as_string(); }
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
		json_string type() const { return _set["type"].as_string(); }
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

	card_database(const char *filename) : _mapping(load(filename)), _sets(parse(_mapping)) { }
	object_collection<card_set> sets() const { return object_collection<card_set>(_sets); }
private:
	static mapping load(const char *filename) {
		return mapping::options()
			.file(file::options(filename).open())
			.write(false)
			.map();
	}
	static json_document parse(mapping &data) {
		auto str = (char *)data.data();
		return json_parse(str, str+data.size());
	}
	mapping _mapping;
	json_document _sets;
};

#endif
