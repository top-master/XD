// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void present()
{
    emitText("case-a", Message("A made-up description with placeholders %1 and %2").insert(theExtremelyLongComparisonValueForAlphaExtraLonger == "x" ? a : b).insert(value()));
    emitText("case-b", Message("A made-up description with placeholders %1 and %2").insert(theExtremelyLongComparisonValueForBetaWithExtraCharacters == "x" ? Text("a") : Text("b")).insert(value()));
    emitText("case-c", Message("A made-up description with placeholders %1 and %2").insert(theExtremelyLongComparisonValueForGammaWithExtraChars == "x" ? Text("an invented long positive branch for this case") : Text("an invented long negative branch for this case")).insert(value()));
    emitText("multi", Message("A made-up description with placeholders %1 and %2").insert(longPredicateWithMeaningfulName == "x" ? Text("positiveValue") : Text("negativeValue"), other == "z" ? Text("y") : Text("n"), comparisonIdentifierForThirdVeryLong == "r" ? Text("p") : Text("q")).insert(value()));
}
