Finite State Machine for C++
============================

A simple, generic, header-only state machine implementation for C++.

Documentation
-------------

Please see the documentation in [fsm.h](./fsm.h) for detailed documentation
about the implemented features and semantics.

Some more information about this component can also be found on our website. See [article 1] for the motivation, and [article 2] for the implementation.

[article 1]: http://wisol.ch/w/articles/2015-01-04-state-machine-intro/
[article 2]: http://wisol.ch/w/articles/2015-03-27-state-machine-impl/

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
