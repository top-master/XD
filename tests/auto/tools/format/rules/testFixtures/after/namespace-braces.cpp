// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

using namespace std;
namespace fs = std::filesystem;

namespace Alpha {
void a();
} // Namespace `Alpha`.

namespace Beta {
void b();
} // Namespace `Beta`.

namespace {
int c;
}

namespace Gamma::Delta {
void d();
} // Namespace `Gamma::Delta`.

namespace Outer {
namespace Inner {
void e();
} // Namespace `Inner`.
} // Namespace `Outer`.

namespace Epsilon {
void f();
} // End of the Epsilon helpers

namespace Zeta {
void g();
// Closes the Zeta helpers.
}

namespace Eta {
void h();
} // Namespace `Eta`.

namespace Theta {
void i();
} // Namespace `Theta`.
