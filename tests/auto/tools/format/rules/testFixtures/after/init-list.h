// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

class Widget
{
public:
    inline Widget() : m_ready(false), m_state(Idle)
                      , m_mode(Fast), m_error(None)
                      , m_count(-1)
    { }

private:
    enum State { Idle, Busy };
    enum Mode { Fast, Slow };
    enum Error { None, Lost };

    bool m_ready;
    State m_state;
    Mode m_mode;
    Error m_error;
    int m_count;
};
