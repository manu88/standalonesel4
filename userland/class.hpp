#pragma once

struct Foo
{
    int a = 11;

    virtual bool test() = 0;
};


class Bar: public Foo
{
    public:
    bool test() override
    {
        return true;
    }
};