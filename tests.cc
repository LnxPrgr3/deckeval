#include "carddb.h"
#include "game.h"
#include <iostream>
#include <memory>

class const_str {
public:
	template <size_t N>
	const_str(const char (&data)[N]) : _data(data), _len(N-1) { }
	const char *c_str() const { return _data; }
	const size_t size() const { return _len; }
private:
	const char *_data;
	size_t _len;
};

std::ostream &operator<<(std::ostream &out, const const_str &str) {
	out.write(str.c_str(), str.size());
	return out;
}

class test {
public:
	test(const const_str &name) : _name(name) { }
	virtual bool operator()() const = 0;
	const const_str &name() const { return _name; }
private:
	const_str _name;
};

template <class Fn>
class test_impl : public test {
public:
	test_impl(const const_str &name, Fn &&t) : test(name), _test(std::forward<Fn>(t)) { }
	bool operator()() const {
		try {
			return _test();
		} catch (...) {
			return false;
		}
	}
private:
	Fn _test;
};

template <class Fn>
test_impl<Fn> make_test(const const_str &name, Fn &&test) {
	return test_impl<Fn>(name, std::forward<Fn>(test));
}

template <class Fn>
std::unique_ptr<test> new_test(const const_str &name, Fn &&t) {
	return std::unique_ptr<test>(new test_impl<Fn>(name, std::forward<Fn>(t)));
}

void run_test(const test &t) {
	std::cout << t.name() << ": " << std::flush << (t() ? "PASS" : "FAIL") << std::endl;
}

static std::unique_ptr<card_database> sets;
static game game;
static player player;

bool test_basic_land(const char *name, const char *result) {
	player.reset_mana();
	auto land = game.add(player, card(sets->find_card(name)));
	land.tap();
	bool res = player.mana_pool() == card_database::cost(result);
	game.remove(land);
	return res;
}

bool test_dual_land(const char *name, const char *result1, const char *result2) {
	player.reset_mana();
	auto land = game.add(player, card(sets->find_card(name)));
	land.tap(0);
	bool res = player.mana_pool() == card_database::cost(result1);
	player.reset_mana();
	land.untap();
	land.tap(1);
	res &= player.mana_pool() == card_database::cost(result2);
	game.remove(land);
	return res;
}

int main(int argc, char *argv[]) {
	game.add(player);
	std::unique_ptr<test> tests[] = {
		new_test("Cards load", []() {
			sets = std::unique_ptr<card_database>(new card_database("cards.json"));
			return true;
		}),
		new_test("Plains taps for white", []() {
			return test_basic_land("Plains", "{W}");
		}),
		new_test("Islands tap for blue", []() {
			return test_basic_land("Island", "{U}");
		}),
		new_test("Swamps tap for black", []() {
			return test_basic_land("Swamp", "{B}");
		}),
		new_test("Mountains tap for red", []() {
			return test_basic_land("Mountain", "{R}");
		}),
		new_test("Forests tap for green", []() {
			return test_basic_land("Forest", "{G}");
		}),
		new_test("Tundra taps for white or blue", []() {
			return test_dual_land("Tundra", "{W}", "{U}");
		}),
		new_test("Underground Sea taps for blue or black", []() {
			return test_dual_land("Underground Sea", "{U}", "{B}");
		}),
		new_test("Badlands taps for black or red", []() {
			return test_dual_land("Badlands", "{B}", "{R}");
		}),
		new_test("Taiga taps for red or green", []() {
			return test_dual_land("Taiga", "{R}", "{G}");
		}),
		new_test("Savannah taps for green or white", []() {
			return test_dual_land("Savannah", "{G}", "{W}");
		}),
		new_test("Scrubland taps for white or black", []() {
			return test_dual_land("Scrubland", "{W}", "{B}");
		}),
		new_test("Volcanic Island taps for blue or red", []() {
			return test_dual_land("Volcanic Island", "{U}", "{R}");
		}),
		new_test("Bayou taps for black or green", []() {
			return test_dual_land("Bayou", "{B}", "{G}");
		}),
		new_test("Plateau taps for red or white", []() {
			return test_dual_land("Plateau", "{R}", "{W}");
		}),
		new_test("Tropical Island taps for green or blue", []() {
			return test_dual_land("Tropical Island", "{G}", "{U}");
		}),
		new_test("Shivan Dragon has all its parts", []() {
			auto x = card(sets->find_card("Shivan Dragon"));
			return x.name() == "Shivan Dragon" &&
			       x.mana_cost() == card_database::cost("{4}{R}{R}") &&
			       x.type() == "Creature — Dragon" &&
			       x.text() == "Flying (This creature can't be blocked except by creatures with flying or reach.)\n{R}: Shivan Dragon gets +1/+0 until end of turn." &&
			       x.power() == "5" &&
			       x.toughness() == "5";
		})
	};
	for(const auto &t: tests) {
		run_test(*t);
	}
}
