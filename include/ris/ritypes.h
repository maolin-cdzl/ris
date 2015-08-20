#pragma once

#include <string>

typedef std::string			uuid_t;
typedef uint64_t			ri_time_t;		// msec

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


ri_time_t ri_time_now();

ri_time_t ri_time_elapsed(ri_time_t last);
