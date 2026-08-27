// License: Apache 2.0 without attribution need.

#ifndef TESTCOVERAGE_H
#define TESTCOVERAGE_H

#include <QtTest/QTestCoverage>

#include "./testenv.h"

/// Lets a test function that needs the internet skip a data row an offline copy of
/// it already proved (such as one served by the bundled server-dummy), but only
/// without internet (TestEnv::hasInternet()); with internet, the row still fails.
class OfflineCoverageRow : public QTestCoverageRow
{
public:
    bool isSkipAllowed() const Q_DECL_OVERRIDE { return !TestEnv::hasInternet(); }
};

#endif // TESTCOVERAGE_H
