// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void record()
{
    results[index].error << buildResult().withDetails(details);
    results[index].error == buildResult().withDetails(details);
    results[index].veryLongErrorMemberName.detailValue << buildResult().withDetails(details);
}
