Finite State Machine for C++
============================

A simple, generic, header-only state machine implementation for C++.

Documentation
-------------

Please see the documentation in `fsm.h` for more detailed documentation about
the implemented features and semantics.

Usage
-----

The implementation is contained in a single header file, `fsm.h`. Simply copy
the file to a convenient place in your project and include it.

Stability
---------

The implementation is already in use in different commercial applications. We
have not found any issues so far. Please report any problems you find.

Tests
-----

Tests can be run with

~~~
cd tests
g++ -std=c++11 -Wall -o tests fsm_test.cpp
./tests
~~~

Contributions
-------------

Contributions are welcome. Please use the Github issue tracker.
