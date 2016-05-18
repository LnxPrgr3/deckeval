#ifndef DECKEVAL_GAME_H
#define DECKEVAL_GAME_H
#include "carddb.h"
#include <vector>

class card : public card_database::card {
public:
	card(const card_database::card &c);
protected:
	std::vector<card_database::cost> _mana;
};

class player {
public:
	const card_database::cost &mana_pool() const;
	void add_mana(const card_database::cost &mana);
	void reset_mana();
private:
	card_database::cost _mana_pool;
};

class permanent : public card {
public:
	permanent(const card &c);
	void tap(int n = 0);
	void untap();
private:
	player *_controller;
	bool _tapped;

	friend class game;
};

class game {
public:
	player *&add(player &player);
	permanent &add(player &player, permanent &&permanent);
	void remove(permanent &p);
private:
	std::vector<player *> _players;
	std::vector<permanent> _battlefield;
};

#endif
