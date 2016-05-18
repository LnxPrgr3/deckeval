#include "game.h"

card::card(const card_database::card &c) : card_database::card(c) {
	if(types().contains("Land")) {
		for(const json_string &subtype: subtypes()) {
			if(subtype == "Plains")
				_mana.emplace_back("{W}");
			else if(subtype == "Island")
				_mana.emplace_back("{U}");
			else if(subtype == "Swamp")
				_mana.emplace_back("{B}");
			else if(subtype == "Mountain")
				_mana.emplace_back("{R}");
			else if(subtype == "Forest")
				_mana.emplace_back("{G}");
		}
	}
}

const card_database::cost &player::mana_pool() const {
	return _mana_pool;
}

void player::add_mana(const card_database::cost &mana) {
	_mana_pool += mana;
}

void player::reset_mana() {
	_mana_pool = card_database::cost();
}

permanent::permanent(const card &c) : card(c) {
	_controller = nullptr;
	_tapped = false;
}

void permanent::tap(int n) {
	if(!_tapped) {
		_tapped = true;
		_controller->add_mana(_mana.at(n));
	}
}

void permanent::untap() {
	_tapped = false;
}

player *&game::add(player &player) {
	_players.push_back(&player);
	return _players.back();
}

permanent &game::add(player &player, permanent &&permanent) {
	_battlefield.push_back(std::move(permanent));
	auto &res = _battlefield.back();
	res._controller = &player;
	return res;
}

void game::remove(permanent &p) {
	auto it = _battlefield.begin();
	it += &p - &*it;
	_battlefield.erase(it);
}
