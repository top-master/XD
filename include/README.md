
**Note** Most of this folder's files are auto-generated, but
are included anyway to NOT depend on Perl being installed.

## Usage
Assume you need both `QString` and `QObject` classes.

Also assume you already know that QObject's header includes QString's header, but
may NOT in a future version.

In such cases, because Qt and/or XD headers are easy to find later, simply do:
```cpp
#include <QtCore/QObject>
```

**But** for most non-Qt headers, which have complex header/class names, you should
include non-Qt header(s) even if Qt header already includes it, like:
```cpp
#include <QtCore/QObject>

#include <stdio.h>
```

Finally if possible, sort your includes by types, in order below:
```cpp
#include "my-app-headers.h"

#include <Qt-headers>

#include <std-headers>


// ... Your code here ...
```
That is because some `std` headers may need a special include-order or some macro to
be defined for them to be usable by Qt, but if you
include Qt headers first, then *usually* there's not much to worry.

## Styling

### Includes:
Place one empty-line after each of said include types, and
two empty-lines after all headers, see example in "Usage" section.

### Code:
Sort code like:
* Place first any `#define` constants,
* Then second are `static` variables,
* Third `static` functions followed by global functions,
* Finally place class-member methods at end, where
  each's in the same order as header **or** group.

Whenever easier, sort class-member methods by related-ness, because
unlike header the source does not force you to sort by `private` vs `public`.

Sort class members like:
* From top-to-bottom start with `public` methods.
* Follwed by `protected` methods.
* Finally `private` methods.
* Then from bottom-to-top like below example, start with `public` variables.
* Follwed by `protected` methods.
* Finally `private` methods.

Basically, it's easy to scroll to file's begin and end, hence
we hide `private` things in the middle.

```cpp
class MyClass {
public:
    MyClass();
    virtual ~MyClass();

    void myFunc();

protected:
    virtual void onMyEvent();

private:
    void myFuncImpl();

private:
    int m_myPrivateVariable;

protected:
    int m_myProtectedVariable;

public:
    int myPublicVariable;
};
```
