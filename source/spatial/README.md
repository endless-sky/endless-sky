![Logo of Spatial](https://github.com/sbougerel/spatial/blob/master/doc/images/spatial_96.png)
# Spatial C++ Library

Generic multi-dimensional containers for C++, with iterators
over orthogonal ranges or nearest neighbors, modeled after the STL.

Web: https://sourceforge.net/projects/spatial/


## Feature overview

- Static (compile-time) or dynamic (run-time) determination of the number of
  dimensions

- Generic design that mimic STL containers, and iterators

- Feature rebalancing k-d tree preventing worst-case run time on many
  operations

- Iteration over orthogonal range with custom predicates and support for points
  or boxes

- Iteration over neighbors, from nearest to furthest with custom metrics

- Iteration over a single dimension, independantly from others

- Heavily unit-tested and production ready


## Minor version change from 2.1 to 2.2

Classes container_traits and metric_traits are removed since they provided no
benefit and inflated the code needlessly. If you used these classes, just remove
them from your code, calling the types from containers or metrics directly.


## Installation

Spatial is a header-only library and all you have to do is to copy the content
of the `src` folder in a folder named `spatial` under the appropriate header
directory for your project or your environment.

Typically on Linux, this just means:

```
$ sudo cp -a src/* /usr/local/include/spatial/
```

Using MSVC++ on Windows, you may install them under:
```
%Visual Studio location%\VC\include\spatial\
```

Once the above is done, you should be able to simply use the library by calling
one of its headers, such as `point_multiset`:

```
#include <spatial/point_multiset.hpp>
```


## Licensing

Spatial C++ Library is provided under the term of the
[Boost 1.0 License](http://www.boost.org/LICENSE_1_0.txt).
Alternatively, see the file `COPYING` provided with your copy of the library.


## Documentation

Currently, the documentation is still hosted on
[Sourceforge](http://spatial.sourceforge.net/). Github-page hosting is coming.

If you which to generate the documentation yourself, first install
[Doxygen](http://www.stack.nl/~dimitri/doxygen/) and
[LaTeX](https://www.latex-project.org/), then:

```
cd doc; doxygen
```

This should generate the `html` and `latex` folders with the documentation.
