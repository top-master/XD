// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

bool pick(int token, int k, const Document *document)
{
    {
        {
            if ((token == LPAREN || token == LBRACK)
                && document->match(k) > k
            ) {
                return true;
            }
            if ((first == second
                    || third == fourth
                    || fifth == sixth
                    || seventh == eighth)
                && ready
            ) {
                return false;
            }
        }
    }
    return false;
}
