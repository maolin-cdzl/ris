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

class RegionRt : public Region {
public:
	ri_time_t				timeval;
};

class Service {
public:
	uuid_t					id;
	std::string				address;
};

class ServiceRt : public Service {
public:
	ri_time_t				timeval;
};

class Payload {
public:
	uuid_t					id;
};

class PayloadRt : public Payload {
public:
	ri_time_t				timeval;
};


ri_time_t ri_time_now();

