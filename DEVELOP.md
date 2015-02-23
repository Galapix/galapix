galapix Development Notes
=========================

External Libraries
------------------

`external/` contains several libraries included here for convenience
and to avoid dependency boat.


WAF
---

To update the ./waf script get the [latest sources](https://code.google.com/p/waf/) and run:

    ./waf-light --tools=boost

Compiling a single target with waf:

    ./waf  -v --targets uitest/sdl_test


Tests
-----

`test/` contains automatic tests based on gtest, they can be run with:

    build/test_galapix

`uitest/` contains interactive tests that have to be manually
executed. Most of them accept command line parameter and provide a
little help text when run.
