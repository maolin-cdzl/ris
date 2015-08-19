#include "ris/pubsub/publisher.h"

struct RIPubArgs {
	Region							region;
	std::vector<std::string>		brokers;
};

static void pub_actor_routine(zsock_t* pipe,void* args);

RIPublisher::RIPublisher() :
	m_actor(nullptr) {
	}


RIPublisher::~RIPublisher() {
	shutdown();
}


int RIPublisher::start(const Region& region,const std::vector<std::string>& brokers) {
	if( nullptr != m_actor )
		return -1;

	RIPubArgs args;
	args.region = region;
	args.brokers = brokers;
	m_actor = zactor_new(&pub_actor_routine,&args);

	return 0;
}

int RIPublisher::shutdown() {
	if( m_actor ) {
		zactor_destroy(&m_actor);
		return 0;
	} else {
		return -1;
	}
}


int RIPublisher::pubService(const uuid_t& reg,uint32_t version,const Service& svc) {
	do {
		if( -1 == zstr_sendm(m_actor,"#svc") )
			break;
		if( -1 == zstr_sendm(m_actor,reg.c_str()) )
			break;
		char ver[32];
		snprintf(ver,sizeof(ver),"%i",version);
		if( -1 == zstr_sendm(m_actor,ver) )
			break;
		if( -1 == zstr_send(m_actor,svc.id.c_str()) )
			break;

		return 0;
	} while(0);

	return -1;
}

int RIPublisher::pubRemoveService(const uuid_t& reg,uint32_t version,const uuid_t& svc) {
	do {
		if( -1 == zstr_sendm(m_actor,"#delsvc") )
			break;
		if( -1 == zstr_sendm(m_actor,reg.c_str()) )
			break;
		char ver[32];
		snprintf(ver,sizeof(ver),"%i",version);
		if( -1 == zstr_sendm(m_actor,ver) )
			break;
		if( -1 == zstr_send(m_actor,svc.c_str()) )
			break;

		return 0;
	} while(0);

	return -1;
}

int RIPublisher::pubPayload(const uuid_t& reg,uint32_t version,const Payload& pl) {
	do {
		if( -1 == zstr_sendm(m_actor,"#pld") )
			break;
		if( -1 == zstr_sendm(m_actor,reg.c_str()) )
			break;
		char ver[32];
		snprintf(ver,sizeof(ver),"%i",version);
		if( -1 == zstr_sendm(m_actor,ver) )
			break;
		if( -1 == zstr_send(m_actor,pl.id.c_str()) )
			break;

		return 0;
	} while(0);

	return -1;
}

int RIPublisher::pubRemovePayload(const uuid_t& reg,uint32_t version,const uuid_t& pl) {
	do {
		if( -1 == zstr_sendm(m_actor,"#delpld") )
			break;
		if( -1 == zstr_sendm(m_actor,reg.c_str()) )
			break;
		char ver[32];
		snprintf(ver,sizeof(ver),"%i",version);
		if( -1 == zstr_sendm(m_actor,ver) )
			break;
		if( -1 == zstr_send(m_actor,pl.c_str()) )
			break;

		return 0;
	} while(0);

	return -1;
}


static zsock_t* pub_actor_create_sock(const std::vector<std::string>& brokers) {
	bool suc = false;
	zsock_t* pub = zsock_new(ZMQ_PUB);

	for(auto it=brokers.begin(); it != brokers.end(); ++it) {
		if( -1 == zsock_connect(pub,"%s",it->c_str()) ) {
			//log it
		} else {
			suc = true;
		}
	}

	if( ! suc ) {
		// log it
		zsock_destroy(&pub);
	}
	return pub;
}

static int pub_actor_broadcast_self(zsock_t* sock,const Region& region) {
	do {
		if( -1 == zstr_sendm(sock,"#reg") )
			break;
		if( -1 == zstr_sendm(sock,region.id.c_str()) )
			break;
		if( -1 == zstr_sendm(sock,region.ids.c_str()) )
			break;
		if( -1 == zstr_send(sock,region.address.c_str()) )
			break;
		return 0;
	} while(0);
	return -1;
}

static int pub_actor_broadcast_self_down(zsock_t* sock,const Region& region) {
	zstr_sendm(sock,"#delreg");
	zstr_send(sock,region.id.c_str());
	return 0;
}


static int calc_timeout(const timespec* last) {
	static const uint64_t BROADCAST_SELF_TIMEOUT	= 5000;

	uint64_t diff = 0;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);

	if( now.tv_nsec < last->tv_nsec ) {
		diff = (now.tv_nsec + 1000000000 - last->tv_nsec) / 1000000;
		now.tv_sec -= 1;
	} else {
		diff = (now.tv_nsec - last->tv_nsec) / 1000000;
	}

	assert( now.tv_sec >= last->tv_sec );
	diff += (now.tv_sec - last->tv_sec) * 1000; 

	if( diff >= BROADCAST_SELF_TIMEOUT ) {
		return 0;
	} else {
		return BROADCAST_SELF_TIMEOUT - diff;
	}
}

static void pub_actor_routine(zsock_t* pipe,void* args) {
	assert(args);
	Region region = ((RIPubArgs*)args)->region;
	std::vector<std::string> brokers = ((RIPubArgs*)args)->brokers;

	zsock_t* pub = pub_actor_create_sock(brokers);

	if( pub ) {
		zsock_signal(pipe,0);
	} else {
		zsock_signal(pipe,-1);
		return;
	}

	pub_actor_broadcast_self(pub,region);
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);

	zpoller_t* poller = zpoller_new(pipe);

	void* reader = nullptr;
	int timeout;
	while(true) {
		timeout = calc_timeout(&ts);
		if( timeout == 0 ) {
			pub_actor_broadcast_self(pub,region);
			clock_gettime(CLOCK_MONOTONIC,&ts);
			timeout = calc_timeout(&ts);
		}
		reader = zpoller_wait(poller,timeout);
		if( reader == nullptr ) {
			if( zpoller_expired(poller) ) {
				pub_actor_broadcast_self(pub,region);
				clock_gettime(CLOCK_MONOTONIC,&ts);
			} else if( zpoller_terminated(poller) ) {
				// log it
				break;
			} else {
				// log it
			}
		} else {
			assert(reader == pub );

			zmsg_t* msg = zmsg_recv(pipe);
			if( zmsg_size(msg) == 1 ) {
				// must be $TERM command
				char* cmd = zmsg_popstr(msg);
				assert( strcmp(cmd,"$TERM") == 0 );
				free(cmd);
				zmsg_destroy(&msg);
				break;
			}

			zmsg_send(&msg,pub);
		}
	}
	
	pub_actor_broadcast_self_down(pub,region);
	zpoller_destroy(&poller);
	zsock_destroy(&pub);
}

