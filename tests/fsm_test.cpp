/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Michael Egli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \author    Michael Egli
 * \copyright wisol technologie GmbH
 * \date      18-Dec-2014
 *
 * \file fsm_test.cpp
 * Test definitions for the state machine implementation.
 */

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <array>
#include <vector>
#include "../fsm.h"

TEST_CASE("Test initial and final pseudo states")
{
	enum class States { Initial, Final };
	using F = FSM::Fsm<States, States::Initial, char>;
	F fsm;
	fsm.add_transitions({
	    {States::Initial, States::Final, 'a', nullptr, nullptr},
	});

	SECTION("Test initial pseudo state")
	{
		REQUIRE(fsm.state() == States::Initial);
		REQUIRE(fsm.is_initial() == true);
	}

	SECTION("Test final state")
	{
		fsm.execute('a');
		REQUIRE(fsm.state() == States::Final);
		REQUIRE(fsm.is_initial() == false);
	}
}

TEST_CASE("Test missing trigger")
{
	enum class States { Initial, Final };
	using F = FSM::Fsm<States, States::Initial, char>;
	F fsm;
	fsm.add_transitions({
	    {States::Initial, States::Final, 'b', nullptr, nullptr},
	});
	REQUIRE(fsm.execute('a') == FSM::Fsm_NoMatchingTrigger);
}

TEST_CASE("Test guards")
{
	SECTION("Test false guard")
	{
		enum class States { Initial, Final };
		using F = FSM::Fsm<States, States::Initial, char>;
		F fsm;
		fsm.add_transitions({
		    {States::Initial, States::Final, 'a', [] { return false; }, nullptr},
		});
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// ensure that the transition to final is not taken (because of the guard).
		REQUIRE(fsm.state() == States::Initial);
	}

	SECTION("Test true guard")
	{
		enum class States { Initial, Final };
		using F = FSM::Fsm<States, States::Initial, char>;
		F fsm;
		fsm.add_transitions({
		    {States::Initial, States::Final, 'a', [] { return true; }, nullptr},
		});
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// ensure that the transition to final is taken (because of the guard).
		REQUIRE(fsm.state() == States::Final);
	}

	SECTION("Test same action with different guards")
	{
		int count = 0;
		enum class States { Initial, Final };
		using F = FSM::Fsm<States, States::Initial, char>;
		F fsm;
		fsm.add_transitions({
		    {States::Initial, States::Final, 'a', [] { return false; }, [&count] { count++; }},
		    {States::Initial, States::Final, 'a', [] { return true; }, [&count] { count = 10; }},
		});
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// ensure that action2 was taken (because of the guard).
		REQUIRE(count == 10);
	}
}

TEST_CASE("Test Transitions")
{
	SECTION("Test multiple matching transitions")
	{
		int count = 0;
		enum class States { Initial, A, Final };
		using F = FSM::Fsm<States, States::Initial, char>;
		F fsm;
		fsm.add_transitions({
		    {States::Initial, States::A, 'a', nullptr, [&count] { count++; }},
		    {States::A, States::A, 'a', nullptr, [&count] { count++; }},
		    {States::A, States::Final, 'a', nullptr, [&count] { count++; }},
		});
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// Ensure that only one action has executed.
		REQUIRE(count == 1);
	}
}

TEST_CASE("Test state machine reset")
{
	int action_count = 0;
	enum class States { Initial, A, Final };
	enum class Triggers { A, B };
	using F = FSM::Fsm<States, States::Initial, Triggers>;
	F fsm;
	fsm.add_transitions({
	    {States::Initial, States::A, Triggers::A, nullptr, nullptr},
	    {States::A, States::Final, Triggers::B, nullptr, nullptr},
	});

	SECTION("Test default reset action")
	{
		REQUIRE(fsm.execute(Triggers::A) == FSM::Fsm_Success);
		REQUIRE(fsm.state() == States::A);
		fsm.reset();
		REQUIRE(fsm.state() == States::Initial);
		REQUIRE(fsm.execute(Triggers::A) == FSM::Fsm_Success);
		REQUIRE(fsm.execute(Triggers::B) == FSM::Fsm_Success);
	}
	SECTION("Test reset to specific state")
	{
		REQUIRE(fsm.execute(Triggers::A) == FSM::Fsm_Success);
		REQUIRE(fsm.state() == States::A);
		fsm.reset(States::Final);
		REQUIRE(fsm.state() == States::Final);
	}
}

TEST_CASE("Test debug function")
{
	int action_count = 0;
	enum class States { Initial, A, Final };
	using F = FSM::Fsm<States, States::Initial, char>;
	F fsm;
	fsm.add_transitions({
	    {States::Initial, States::A, 'a', nullptr, nullptr},
	    {States::A, States::Final, 'b', nullptr, nullptr},
	});

	SECTION("Test enable debugging function.")
	{
		States dbg_from;
		States dbg_to;
		char dbg_tr = 0;
		fsm.add_debug_fn([&](States from, States to, char tr) {
			dbg_from = from;
			dbg_to = to;
			dbg_tr = tr;
		});
		fsm.execute('a');
		REQUIRE(dbg_from == States::Initial);
		REQUIRE(dbg_to == States::A);
		REQUIRE(dbg_tr == 'a');
	}

	SECTION("Test disable debugging function.")
	{
		int dbg_from = 0;
		int dbg_to = 0;
		char dbg_tr = 0;
		fsm.reset();
		fsm.add_debug_fn(nullptr);
		REQUIRE(dbg_from == 0);
		REQUIRE(dbg_to == 0);
		REQUIRE(dbg_tr == 0);
	}
}

TEST_CASE("Test single argument add_transitions function")
{
	enum class States { Initial, A, Final };
	using F = FSM::Fsm<States, States::Initial, char>;
	F fsm;

	SECTION("Test raw array")
	{
		F::Trans v[] = {
		    {States::Initial, States::A, 'a', nullptr, nullptr},
		    {States::A, States::Final, 'b', nullptr, nullptr},
		};
		fsm.add_transitions(&v[0], &v[2]);
		fsm.execute('a');
		fsm.execute('b');
		REQUIRE(fsm.state() == States::Final);
	}

	SECTION("Test vector")
	{
		std::vector<F::Trans> v = {
		    {States::Initial, States::A, 'a', nullptr, nullptr},
		    {States::A, States::Final, 'b', nullptr, nullptr},
		};
		fsm.add_transitions(v);
		fsm.execute('a');
		fsm.execute('b');
		REQUIRE(fsm.state() == States::Final);
	}

	SECTION("Test initializer list")
	{
		fsm.add_transitions({
		    {States::Initial, States::A, 'a', nullptr, nullptr},
		    {States::A, States::Final, 'b', nullptr, nullptr},
		});
		fsm.execute('a');
		fsm.execute('b');
		REQUIRE(fsm.state() == States::Final);
	}
}

TEST_CASE("Test int as type for states")
{
	int INITIAL = 1;
	int A = 2;
	int FINAL = 3;
	using F = FSM::Fsm<int, 1, char>;
	F fsm;
	fsm.add_transitions({
	    {INITIAL, A, 'a', nullptr, nullptr}, {A, A, 'b', nullptr, nullptr},
	    {A, FINAL, 'c', nullptr, nullptr},
	});
	REQUIRE(fsm.state() == INITIAL);
	fsm.execute('a');
	REQUIRE(fsm.state() == A);
	fsm.execute('b');
	REQUIRE(fsm.state() == A);
	fsm.execute('c');
	REQUIRE(fsm.state() == FINAL);
}

TEST_CASE("Test type safe triggers")
{
	enum class States { Initial, A, Final };
	enum class Triggers { A, B };
	using F = FSM::Fsm<States, States::Initial, Triggers>;
	F fsm;
	fsm.add_transitions({
	    {States::Initial, States::A, Triggers::A, nullptr, nullptr},
	    {States::A, States::Final, Triggers::B, nullptr, nullptr},
	});
	fsm.execute(Triggers::A);
	fsm.execute(Triggers::B);
	REQUIRE(fsm.state() == States::Final);
}

TEST_CASE("Test passing an argument to an action")
{
	struct PushMe { int i; };
	using PushMe_t = std::shared_ptr<PushMe>;
	PushMe_t push_me = std::make_shared<PushMe>();
	enum class States { Initial };
	enum class Triggers { A };
	int res = 0;
	using F = FSM::Fsm<States, States::Initial, Triggers>;
	F fsm;
	fsm.add_transitions({
	    {States::Initial, States::Initial, Triggers::A, nullptr, std::bind([&](PushMe_t p){res=p->i;}, push_me)},
	});
	push_me->i = 42;
	fsm.execute(Triggers::A);
	REQUIRE(res == 42);
	push_me->i = 43;
	fsm.execute(Triggers::A);
	REQUIRE(res == 43);
}
