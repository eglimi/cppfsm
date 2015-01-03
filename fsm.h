#ifndef FSM_H
#define FSM_H

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
 * \file fsm.h
 * Generic implementation of a finite state machine (FSM).
 *
 * Overview
 * --------
 *
 * A finite state machine is a machine that can be in any of a finite number of
 * states. At a time, it can only be in one state. In order to change the
 * machine to another state, a transition can be executed.
 *
 * A transition is initiated by a trigger. A trigger to the machine is only
 * evaluated for outgoing transitions of the current state. A transition can
 * have a guard and a transition action associated with it. The guard is a
 * function that checks if the transition should be executed. The transition
 * action is a function that is called when the transition is effectively
 * executed.
 *
 * If a trigger is sent to the machine, and more than one guard evaluate to
 * `true`, then one of the transitions is randomly chosen.
 *
 * A state can have any number of incoming and / or outgoing transitions
 * associated with it, but at least one transition must be present. There are
 * two special states.
 *
 * - Initial pseudo state (no incoming transition, one or more outgoing
 *   transitions).
 * - Final pseudo state (one or more incoming transitions, no outgoing
 *   transition).
 *
 * The initial pseudo state must be present in each FSM.
 *
 * Important concepts
 * ------------------
 *
 * Some concepts are important to remember when defining a state machine.
 *
 * - Each FSM must define a transition from the initial pseudo state to
 *   another state.
 * - If a guard is executed multiple times successively, it must return the
 *   same value every time (no side-effects in guards).
 * - A FSM is a purely reactive, and therefore passive component. In order to
 *   execute some actions, it must receive a trigger.
 *
 * Semantics
 * ---------
 *
 * The following semantic is implemented when a machine receives a trigger.
 *
 * - Consume a trigger.
 * - Check if the trigger potentially initiates some transitions from the
 *   current state. Otherwise return.
 * - For each transition found, check if its guard evaluates to `true`. Return
 *   if none is found.
 * - Execute the transition action of one (and only one) of the selected
 *   transition.
 * - Change current state to the state where the transition points to.
 *
 * Limitations
 * -----------
 *
 * This FSM implementation does not implement all concepts found in all
 * definitions of state machines (notably the UML definition). Some parts that
 * are omitted are.
 *
 * - entry actions, exit actions, do actions
 * - hierarchical nested states
 * - orthogonal regions
 *
 * Implementation
 * --------------
 *
 * This implementation can be used to implement a FSM. It enforces the
 * semantics described above, while being non-intrusive. All guards and actions
 * are defined in the client.
 *
 * The state machine and transitions can be conveniently defined with an array
 * of FSM::Trans structs. This makes the structure of the FSM very clear.
 *
 * C++11
 * -----
 *
 * The implementation uses some C++11 features. Therefore, in order to use the
 * code, the compiler must support theses features and C++11 must be enabled.
 *
 * Currently, the following features are used.
 *
 * - std::function
 * - nullptr
 *
 * Note that for older compilers (notably gcc), std::function has been included
 * for some time in the tr1 header. For such compilers, the code can be changed
 * to use `std::tr1::function` and `NULL` respectively.
 *
 * Debug
 * -----
 *
 * It is possible to add a debug function in order to track state changes. The
 * debug function is either a `nullptr`, or of type `debugFn`. When the
 * function is defined, it is invoked with the `from_state`, `to_state`, and
 * `trigger` arguments whenever a state change happens.
 *
 * \code
 * // Define debug function
 * void dbg_fsm(int from_state, int to_state, char trigger) {
 *   std::cout << "changed from " << from_state << " to " << to_state << " with trigger " << trigger;
 * }
 * // Enable debug
 * fsm.add_debug_fn(dbg_fsm);
 * // Disable debug
 * fsm.add_debug_fn(nullptr);
 * \endcode
 *
 * Example
 * -------
 *
 * The following example implements this simple state machine.
 *
 * \code
 *   ---------------                    ----------                              -------------
 *   | Fsm_Initial | -- 'a'/action1 --> | stateA | -- [guard2]'b' / action2 --> | Fsm_Final |
 *   ---------------                    ----------                              -------------
 * \endcode
 *
 * \code
 *   void action1() { std::cout << "perform custom action 1\n"; }
 *   bool guard2() const { return true; }
 *   void action2() { std::cout << "perform custom action 2\n"; }
 *   const int stateA = 1;
 *   FSM::Trans transitions[] =
 *   {
 *     // from state     , to state      , trigger, guard           , action
 *     { FSM::Fsm_Initial, stateA        , 'a'    , nullptr         , action1 },
 *     { stateA          , FSM::Fsm_Final, 'b'    , []{return true} , action2 },
 *   };
 *
 *   FSM::Fsm fsm;
 *   fsm.add_transitions(&transitions[0], &transitions[0]+2);
 *   fsm.init();
 *   assert(is_initial());
 *   fsm.execute('a');
 *   assert(stateA == fsm.state());
 *   fsm.execute('b');
 *   assert((int)FSM::Fsm_Final == fsm.state());
 *   assert(is_final());
 *   fsm.reset();
 *   assert(is_initial());
 * \endcode
*/

// Includes
#include <limits.h>
#include <map>
#include <vector>
#include <functional>

// Forward declarations

namespace FSM {

/**
 * A list of predefined pseudo states.
 */
enum Predefined_States {
	Fsm_Initial = INT_MAX-1,
	Fsm_Final = INT_MAX,
};

enum Fsm_Errors {
	// Success
	Fsm_Success = 0,

	// Warnings
	// The current state has not such trigger associated.
	Fsm_NoMatchingTrigger,

	// Errors
	// The state machine has not been initialized. Call init().
	Fsm_NotInitialized,
};

// Defines the function prototype for a guard function.
typedef std::function<bool()> guardFn;
// Defines the function prototype for an action function.
typedef std::function<void()> actionFn;
// Defines the function prototype for a debug function.
// Parameters are: from_state, to_state, trigger
typedef std::function<void(int,int,char)> debugFn;

/**
 * Defines a transition between two states.
 */
struct Trans {
	int from_state;
	int to_state;
	char trigger;
	guardFn guard;
	actionFn action;
};

/**
 * An generic finite state machine (FSM) implementation.
 */
class Fsm {

	// Definitions for the structure that holds the transitions.
	// For good performance on state machines with many transitions, transitions
	// are stored for each `from_state`:
	//   map<from_state, vector<Trans> >
	typedef std::vector<Trans> transition_elem_t;
	typedef std::map<int, transition_elem_t> transitions_t;
	typedef transitions_t::const_iterator transitions_it;
	transitions_t m_transitions;
	// Current state.
	int m_cs;
	bool m_initialized;
	debugFn m_debug_fn;

public:

	// Constructor.
	Fsm() : m_transitions(), m_cs(0), m_initialized(false), m_debug_fn(nullptr) {}

	/**
	 * Initializes the FSM.
	 *
	 * This sets the current state to Fsm_Initial.
	 * Once the fsm has been initialized, calling this function has no effect.
	 */
	void init() {
		if(!m_initialized) {
			m_cs = Fsm_Initial;
			m_initialized = true;
		}
	}

	/**
	 * Set the machine to uninitialized and the state to Fsm_Initial.
	 *
	 * This method can be called at any time. After a reset, init() must be
	 * called in order to use the machine.
	 */
	void reset() {
		m_cs = Fsm_Initial;
		m_initialized = false;
	}

	/**
	 * Add a set of transition definitions to the state machine.
	 *
	 * This function can be called multiple times at any time. Added
	 * transitions cannot be removed from the machine.
	 */
	template<typename _InputIt>
	void add_transitions(_InputIt start, _InputIt end) {
		_InputIt it = start;
		for(; it != end; ++it) {
			// Add element in the transition table
			m_transitions[(*it).from_state].push_back(*it);
		}
	}

	/**
	 * Adds a function that is called on every state change. The type of the
	 * function is `debugFn`. It has the following parameters.
	 *
	 * - from_state (int)
	 * - to_state (int)
	 * - trigger (char)
	 *
	 * It can be used for debugging purposes. It can be enabled and disabled at
	 * runtime. In order to enable it, pass a valid function pointer. In order
	 * to disable it, pass `nullptr` to this function.
	 */
	void add_debug_fn(debugFn fn) {
		m_debug_fn = fn;
	}

	/**
	 * Execute the given trigger according to the semantics defined for this
	 * state machine.
	 *
	 * Returns the status of the execute operation. Fsm_Success is 0.
	 */
	Fsm_Errors execute(char trigger) {
		if(not m_initialized) {
			return Fsm_NotInitialized;
		}

		Fsm_Errors err_code = Fsm_NoMatchingTrigger;

		transitions_it state_transitions = m_transitions.find(m_cs);
		if(state_transitions == m_transitions.end()) {
			return err_code; // No transition from current state found.
		}

		// iterate the transitions
		const transition_elem_t& active_transitions = state_transitions->second;
		transition_elem_t::const_iterator it = active_transitions.begin();
		for(; it != active_transitions.end(); ++it) {

			// Check if trigger matches.
			if(trigger != (*it).trigger) continue;
			err_code = Fsm_Success;

			// Check if guard exists and returns true.
			if((*it).guard && (not(*it).guard())) continue;

			// Now we have to take the action and set the new state.
			// Then we are done.

			// Check if action exists and execute it.
			if((*it).action != 0) {
				(*it).action(); //execute action
			}
			m_cs = (*it).to_state;
			if(m_debug_fn) {
				m_debug_fn((*it).from_state, (*it).to_state, trigger);
			}
			break;
		}

		return err_code;
	}

	/**
	 * Returns the current state;
	 */
	int state() const { return m_cs; }
	/**
	 * Returns whether the current state is the initial state.
	 */
	bool is_initial() const { return m_cs == Fsm_Initial; }
	/**
	 * Returns whether the current state is the final state.
	 */
	bool is_final() const { return m_cs == Fsm_Final; }
};

} // end namespace FSM

#endif // FSM_H
