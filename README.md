# Soufflé with Eager Evaluation

This fork extends the Datalog system Soufflé with eager evaluation, a concurrent Datalog evaluation algorithm that is faster than traditional semi-naive Datalog evaluation on some workloads.
Unlike semi-naive Datalog evaluation, it does not batch derived tuples before exploring their consequences; instead, it _eagerly_ processes derived tuples with the help of a work-stealing thread pool.

Enable eager evaluation via the `--eager-eval` flag (it works only when using Soufflé's compiler mode; the interpreter mode is not supported).

## Additional Dependencies

In addition to the standard dependencies for Soufflé listed below, our implementation of eager evaluation requires [the oneTBB library](https://github.com/oneapi-src/oneTBB) (v2021.13.0 is known to work, but other versions will likely work too).

## Implementation Overview

Our extension amounts to roughly 500 additional lines of C++; see [the diff](https://github.com/souffle-lang/souffle/compare/master...aaronbembenek:souffle:eager-eval) with our changes.
We briefly overview our modifications.

In `src/ast2ram/utility/SipsMetric.(h|cpp)` we added a new `delta` Sideways Information Passing Strategy ([SIPS](https://souffle-lang.github.io/handtuning#sideways-information-passing-strategy)) that reorders the premises in a rule body so that the delta atom comes first, and the order of all other premises is unchanged.
Our implementation of eager evaluation always uses this SIPS.

In `src/include/souffle/datastructure/EagerEval.h` we define a templated (multi)set that supports concurrent reads and writes.
In general, a relation might be simultaneously read and written during eager evaluation.
Soufflé's built-in datastructures do not support this type of concurrency, as reading and writing never happens simultaneously in traditional semi-naive evaluation.

We extend `src/synthesiser/Relation.(h|cpp)` with the logic for constructing an indexed datastructure representing a relation, where an index might be backed by a standard Soufflé built-in datastructure _or_ one of our custom (multi)sets that plays well with eager evaluation.
For eager evaluation to work correctly, not every index needs to be backed by a custom datastructure; rather, only those indices that are read and written in the same stratum.
Since Soufflé's built-in datastructures are faster than our custom one, we use the built-in ones when possible.

Most of our additions occur in `src/synthesiser/Synthesiser.(h|cpp)`, the code that takes RAM instructions (one of Soufflé's intermediate representations) and generates C++ code.
Whereas base Soufflé uses OpenMP's parallel `for` loops to parallelize evaluation, our extension uses oneTBB's `parallel_for_each` constructs (to effect parallelization while a rule is being evaluated) and `task_group` objects (to parallelize the evaluation of multiple rules, each one specialized on a newly derived tuple).
Each Datalog rule maps to its own function in the C++ code.
Within each function, the code for rule evaluation is similar to the code for semi-naive evaluation of the rule; however, we have to tweak the way some RAM instructions are translated (look for conditionals involving `glb.config().has("eager-eval")`).
Most changes happen because we need to change the way parallelism is implemented or because the semi-naive rules insert tuples into auxiliary relations that do not exist in eager evaluation.

## Limitations

Our extension to Soufflé supports eager evaluation only when a Soufflé program is compiled; it does not support eager evaluation for interpretation.

While our eager evaluation extension is general enough to support large and varied Soufflé programs, it does not yet support all RAM instructions; trying to codegen unsupported instructions should result in an assertion failure.
Additionally, we have not tried to integrate the code for eager evaluation with ancillary Soufflé modes, such as provenance tracking or performance profiling.

---

[Original Soufflé README]

# Welcome!

This is the official repository for the [Soufflé](https://souffle-lang.github.io) language project.
The Soufflé language is similar to Datalog (but has terms known as records), and is frequently used as a
domain-specific language for analysis problems.

[![License: UPL](https://img.shields.io/badge/License-UPL--1.0-blue.svg)](https://github.com/souffle-lang/souffle/blob/master/LICENSE)
[![CI-Tests](https://github.com/souffle-lang/souffle/actions/workflows/CI-Tests.yml/badge.svg)](https://github.com/souffle-lang/souffle/actions/workflows/CI-Tests.yml)
[![MSVC-CI-Tests](https://github.com/souffle-lang/souffle/actions/workflows/VS-CI-Tests.yml/badge.svg)](https://github.com/souffle-lang/souffle/actions/workflows/VS-CI-Tests.yml)
[![codecov](https://codecov.io/gh/souffle-lang/souffle/branch/master/graph/badge.svg)](https://codecov.io/gh/souffle-lang/souffle)

## Features of Soufflé

*   Efficient translation to parallel C++ of Datalog programs (CAV'16, CC'16)

*   Efficient interpretation using de-specialization techniques (PLDI'21)

*   Specialized data structure for relations (PACT'19, PPoPP'19, PMAM'19) with optimal index selection (VLDB'18)

*   Extended semantics of Datalog, e.g., permitting unbounded recursions with numbers and terms

*   Simple component model for Datalog specifications

*   Recursively defined record types/ADTs (aka. constructors) for tuples

*   User-defined functors

*   Strongly-typed types for safety

*   Subsumption

*   Aggregation

*   Choice Construct (APLAS'21)

*   Extended I/O system for relations (including SQLITE3 interfaces)

*   C++/SWIG interfaces

*   Provenance/Debugging (TOPLAS'20)

*   Profiling tools


## How to get Soufflé

Use git to obtain the source code of Soufflé.

    $ git clone git://github.com/souffle-lang/souffle.git

Build instructions can be found [here](https://souffle-lang.github.io/build).

## Legacy code

If you have written code for an older version of Souffle, please use the command line flag `--legacy`.
Alternatively, please add the following line to the start of your source-code:

```
.pragma "legacy"
```

## Issues and Discussions 

Use either the [issue list](https://github.com/souffle-lang/souffle/issues) for

- bug reporting

- enhancements

- documentation

- proposals

- releases

- compatibility issues

- refactoring.

or use the [discussions](https://github.com/souffle-lang/souffle/discussions) bulletin board to engage with other community members and for asking questions you’re wondering about.

## How to contribute

Issues and bug reports for Souffle are found in the [issue list](https://github.com/souffle-lang/souffle/issues).
This list is also where new contributors may find extensions / bug fixes to work on.

To contribute in this repo, please open a pull request from your fork of this repository.
The general workflow is as follows.

1. Find an issue in the issue list.

2. Fork the [souffle-lang/souffle](http://github.com/souffle-lang/souffle.git) repo.

3. Push your changes to a branch in your forked repo.

4. Submit a pull request to souffle-lang/souffle from your forked repo.

Our continuous integration framework enforces coding guidelines with the help of clang-format and clang-tidy.

For more information on building and developing Souffle, please read the [developer tutorial](https://souffle-lang.github.io/development).

## [Home Page](https://souffle-lang.github.io)

## [Documentation](https://souffle-lang.github.io/docs.html)

## [Contributors](https://souffle-lang.github.io/contributors)

## [Issues](https://github.com/souffle-lang/souffle/issues)

## [License](https://github.com/souffle-lang/souffle/blob/master/licenses/SOUFFLE-UPL.txt)

## For academics

If you use our work, please cite our work. A list of publications can be found [here](https://souffle-lang.github.io/publications). The main publications are:
 * Herbert Jordan, Bernhard Scholz, Pavle Subotić: Souffle: On Synthesis of Program Analyzers. CAV 2016.
 * Bernhard Scholz, Herbert Jordan, Pavle Subotić, Till Westmann: On fast large-scale program analysis in Datalog. CC 2016: 196-206
