#pragma once

#include <string>

typedef std::string			uuid_t;

class Region {
public:
	uuid_t					id;
	std::string				ids;
	std::string				address;
};

class Service {
public:
	uuid_t					id;
};

class Payload {
public:
	uuid_t					id;
};


