#pragma once
class ObjectFactory;
class PlatformExpert{
public:
    bool init(ObjectFactory* factory);
private:
    ObjectFactory* _factory = nullptr;
};