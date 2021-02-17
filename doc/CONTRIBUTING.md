Contributing Guidelines
===

Contributions welcome!

`gbr2ngc` is still very much a work in progress but the following are a set of loose guidelines
to follow when contributing code or other work.

Please see the [Code of Conduct](CODE_OF_CONDUCT.md) for acceptable behavior.
In general, we want to foster a welcoming and respectful environment.

Issue Tracking
---

For open issues, check the [list of GitHub issues](https://github.com/abetusk/gbr2ngc/issues) for this project.
It's encouraged, but not necessary, to announce intent to contribute to an issue.
Announce intent by adding a response to an issue, asking for any clarification if need be.

Issues have tags to help with the type, scope and difficulty of the issue.
As a preference, high priority, low difficulty bugs should be prioritized but
obviously any issues that you're interested in resolving are welcome.

When claiming an issue, assign the `in progress` tag to it along with a timeline
of expected work.
If the timeline has been exceeded, other contributors wanting to work on that issue
will be encouraged to do so.
It will be up to the `gbr2ngc` maintainer to determine whether an `in progress` tag should be removed
that has passed it's deadline.

If a bug, feature or whatever else is desired, create an issue.
The specificity of the description is that it provides enough detail so that someone else with good knowledge
of the project can resolve it with minimal additional clarification or outside help.

For bug reporting, all details involved in reliably reproducing the error are strongly encouraged to be shared.
For example, the command line argument that was used to trigger the bug, a minimal input data file that can
be attached to the issue, relevant detail about the system the program was run on and any other pertinent details
to be able to reproduce the issue.


Contributing to Source Code
---

1. Create your own [fork of the main repo](https://help.github.com/en/articles/fork-a-repo).
3. Once work has been completed, create a [pull request](https://help.github.com/en/articles/creating-a-pull-request-from-a-fork).

Code should follow [code style](#code_style) of the project.

For pull requests that have no errors in them:

* First time contributors issuing a pull request that does not follow the [code style](#code_style) will be thanked but
  given polite feedback that subsequent pull requests should follow the [code style](#code_style), with the pull request
  code formatted to obey the [code style](#code_style) by the person merging the pull request.
* Second time contributors issuing a pull request that does not follow the [code style](#code_style) will be thanked but
  given polite feedback that subsequent pull requests should follow the [code style](#code_style) with a warning that future
  pull requests will not be merged unless the [code style](#code_style) is used. The pull request will be
  formatted to obey the [code style](#code_style) by the person merging the pull request.
* Contributors issuing a pull request that have contributed more than twice in the past will be thanked but will not
  have their pull requests merged until the issuing contributor edits the code to comply with the [code style](#code_style).

Code Style
---

### Prefer two space indentation to tab or four spaces

example:

```
void f(void) {
  printf("two space indentation, please\n");
}
```

### Prefer curly braces on the same line as control statements and function headings

example:

```
  for (i=0; i<10; i++) {
    ...
  }
```

### Always use curly braces even when you don't need to

example:

```
  if (its_true) {
    printf("please still use curlys, even if you don't need to\n");
  }
```

### Place small conditional statements on a single line

example:

```
  if (meow_count>10) { feed_cat = 1; }
```

### Prefer `else` conditionals on their own line

example:

```
  if (bowl==EMPTY) {
    feed_cat = 1;
    pet_cat = 0;
  }
  else if (meow_count > 2) {
    feed_cat = 0;
    pet_cat = 1;
  }
  else {
    change_litter = 1;
  }
```

### Prefer ternary conditional assignment over if/else assignment for small statements

example:

```
  meow_at_cat = ( (meow_count < 1) ? 1 : 0 );
```

### Prefer putting comments at top of blocks and prefer to not use inline comments

example:

```
  // cats do not understand context free grammars
  // so we have to infer what their intent is from
  // their meows.
  //
  if (meow_count < 5) {
    pet_cat = 1;
  }

  // the cat must be hungry
  //
  else if (meow_count < 10) {
    feed_cat = 1;
  }
```


### No trailing whitespace at end of lines

### Align assignments for readability

example:

```
  int x00 = -1, x01 = 0,
      x10 =  1, x11 = 1;
```

### Prefer `g` prefix for global variables

example:

```
double gRadius;
```

### Prefer lower camelCase for variable names

example:

```
int myLocalVariable=0;
```

### Use the `m_` prefix for member variables in C++ classes

example:

```
class Aperture_realization {
  ...
  Paths m_macro_path;
  ...
}; 
```

### Use C++ features sparingly if at all

This includes not making deep class hierarchies,
over-using inheritance, heavily relying on virtual functions,
templates, `shared_ptrs`, auto, exceptions, asserts, lambdas
or any of the host of other features.

Do not use the `private` class directive, make all member variables
and methods public.

Using these when needed for third party libraries
is acceptable but introducing these in new code is not.

### Prefer readability over any of the above roles

The purpose of code is to be correct and readable.
If the above rules would make code less readable then
some other formatting method, prefer readability over
rigorous adherence to the above rules.

**NOTE**: The code violates the above rules all over the place.
The coding style is aspirational in the sense future additions
should try to follow the above criteria.

Testing
---

Testing is currently a work in progress.
Eventually there will be automated tests that should pass before accepting pull requests.

In the mean time, verifying program correctness by hand and with example files should be done.

License
---

All code, both existing and to be contributed, must be under a free/libre GPL compatible license.
All data to be contributed should be under a free/libre license (for example, `CC0`, `CC-BY`, `CC-BY-SA`).


There are different code licenses for different files.
The default is GPLv3 but each source should have it's own license information in the header.

Contributors are encouraged to add their name to the README for credit and copyright attribution.
