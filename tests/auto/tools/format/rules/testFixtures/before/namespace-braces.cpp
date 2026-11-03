// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

using namespace std;
namespace fs = std::filesystem;

namespace Alpha { void a(); }

namespace Beta {
void b();
} // namespace Beta

namespace {
int c;
}

namespace Gamma::Delta {
void d();
}

namespace Outer { namespace Inner {
void e();
} }

namespace Epsilon {
void f();
} // End of the Epsilon helpers

namespace Zeta {
void g();
// Closes the Zeta helpers.
}

namespace Eta {
void h();
}  // end of namespace eta

namespace Theta {
void i();
} // TODO: split
