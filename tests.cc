#include "carddb.h"
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

int main(int argc, char *argv[]) {
	std::unique_ptr<card_database> sets;
	std::unique_ptr<test> tests[] = {
		new_test("Cards load", [&sets]() {
			sets = std::unique_ptr<card_database>(new card_database("cards.json"));
			return true;
		})
	};
	for(const auto &t: tests) {
		run_test(*t);
	}
}
