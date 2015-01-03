/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 wisol technologie GmbH
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
#include "../fsm.h"

TEST_CASE("Test Initialization")
{
	FSM::Fsm fsm;
	REQUIRE(fsm.execute('a') == FSM::Fsm_NotInitialized);
}

TEST_CASE("Test initial and final pseudo states")
{
	FSM::Fsm fsm;
	FSM::Trans transitions[] = {
		{FSM::Fsm_Initial, FSM::Fsm_Final, 'a', nullptr, nullptr},
	};
	fsm.add_transitions(&transitions[0], &transitions[0]+1);
	fsm.init();

	SECTION("Test initial pseudo state") {
		REQUIRE(fsm.state() == (int)FSM::Fsm_Initial);
		REQUIRE(fsm.is_initial() == true);
		REQUIRE(fsm.is_final() == false);
	}

	SECTION("Test initial pseudo state") {
		fsm.execute('a');
		REQUIRE(fsm.state() == (int)FSM::Fsm_Final);
		REQUIRE(fsm.is_initial() == false);
		REQUIRE(fsm.is_final() == true);
	}
}

TEST_CASE("Test missing trigger")
{
	FSM::Fsm fsm;
	FSM::Trans transitions[] = {
		{FSM::Fsm_Initial, FSM::Fsm_Final, 'b', nullptr, nullptr},
	};
	fsm.add_transitions(&transitions[0], &transitions[0]+1);
	fsm.init();
	REQUIRE(fsm.execute('a') == FSM::Fsm_NoMatchingTrigger);
}

TEST_CASE("Test guards")
{
	SECTION("Test false guard") {
		FSM::Fsm fsm;
		FSM::Trans transitions[] = {
			{FSM::Fsm_Initial, FSM::Fsm_Final, 'a', []{return false;}, nullptr},
		};
		fsm.add_transitions(&transitions[0], &transitions[0]+1);
		fsm.init();
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// ensure that the transition to final is not taken (because of the guard).
		REQUIRE(fsm.state() == (int)FSM::Fsm_Initial);
	}

	SECTION("Test true guard") {
		FSM::Fsm fsm;
		FSM::Trans transitions[] = {
			{FSM::Fsm_Initial, FSM::Fsm_Final, 'a', []{return true;}, nullptr},
		};
		fsm.add_transitions(&transitions[0], &transitions[0]+1);
		fsm.init();
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// ensure that the transition to final is taken (because of the guard).
		REQUIRE(fsm.state() == (int)FSM::Fsm_Final);
	}

	SECTION("Test same action with different guards") {
		int count = 0;
		FSM::Fsm fsm;
		FSM::Trans transitions[] = {
			{FSM::Fsm_Initial, FSM::Fsm_Final, 'a', []{return false;}, [&count]{count++;}},
			{FSM::Fsm_Initial, FSM::Fsm_Final, 'a', []{return true; }, [&count]{count = 10;}},
		};
		fsm.add_transitions(&transitions[0], &transitions[0]+2);
		fsm.init();
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// ensure that action2 was taken (because of the guard).
		REQUIRE(count == 10);
	}
}

TEST_CASE("Test Transitions")
{
	SECTION("Test multiple matching transitions") {
		int count = 0;
		const int stateA = 1;
		FSM::Fsm fsm;
		FSM::Trans transitions[] = {
			{FSM::Fsm_Initial, stateA        , 'a', nullptr, [&count]{count++;}},
			{stateA          , stateA        , 'a', nullptr, [&count]{count++;}},
			{stateA          , FSM::Fsm_Final, 'a', nullptr, [&count]{count++;}},
		};
		fsm.add_transitions(&transitions[0], &transitions[0]+3);
		fsm.init();
		REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
		// Ensure that only one action has executed.
		REQUIRE(count == 1);
	}
}

TEST_CASE("Test state machine reset")
{
	int action_count = 0;
	const int stateA = 1;
	FSM::Fsm fsm;
	FSM::Trans transitions[] = {
		{FSM::Fsm_Initial, stateA        , 'a', nullptr, nullptr},
		{stateA          , FSM::Fsm_Final, 'b', nullptr, nullptr},
	};
	fsm.add_transitions(&transitions[0], &transitions[0]+2);
	fsm.init();
	REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
	REQUIRE(fsm.state() == stateA);
	fsm.reset();
	REQUIRE(fsm.state() == (int)FSM::Fsm_Initial);
	REQUIRE(fsm.execute('a') == FSM::Fsm_NotInitialized);
	fsm.init();
	REQUIRE(fsm.execute('a') == FSM::Fsm_Success);
	REQUIRE(fsm.execute('b') == FSM::Fsm_Success);
	REQUIRE(fsm.is_final() == true);
}

TEST_CASE("Test debug function")
{
	int action_count = 0;
	const int stateA = 1;
	FSM::Fsm fsm;
	FSM::Trans transitions[] = {
		{FSM::Fsm_Initial, stateA        , 'a', nullptr, nullptr},
		{stateA          , FSM::Fsm_Final, 'b', nullptr, nullptr},
	};
	fsm.add_transitions(&transitions[0], &transitions[0]+2);
	fsm.init();

	SECTION("Test enable debugging function.") {
		int dbg_from = 0;
		int dbg_to = 0;
		char dbg_tr = 0;
		fsm.add_debug_fn([&dbg_from, &dbg_to, &dbg_tr](int from, int to, char tr){ dbg_from = from; dbg_to = to; dbg_tr = tr; });
		fsm.execute('a');
		REQUIRE(dbg_from == FSM::Fsm_Initial);
		REQUIRE(dbg_to == stateA);
		REQUIRE(dbg_tr == 'a');
	}
	
	SECTION("Test disable debugging function.") {
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

