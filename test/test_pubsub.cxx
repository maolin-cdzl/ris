#include "mock_defines.h"
#include "test_helper.h"

#include "region/publisher.h"
#include "tracker/subscriber.h"

extern zloop_t* g_loop;

static const char* PUB_ADDRESS = "inproc://test-pub";

struct PubTaskCounter {
	size_t				region_count;
	size_t				rm_region_count;
	size_t				service_count;
	size_t				rm_service_count;
	size_t				payload_count;
	size_t				rm_payload_count;
	
	PubTaskCounter() :
		region_count(0),
		rm_region_count(0),
		service_count(0),
		rm_service_count(0),
		payload_count(0),
		rm_payload_count(0)
	{
	}
};

static PubTaskCounter make_pub_tasks(const std::shared_ptr<RIPublisher>& pub,std::shared_ptr<TaskRunner>& runner,size_t region_count,size_t service_count,size_t payload_count) {
	PubTaskCounter counter;

	std::list<Region> regions;
	std::list<Service> services;
	std::list<Payload> payloads;

	for(size_t i=0; i < region_count ; ++i) {
		Region region;
		region.id = newUUID();
		region.idc = "test";
		region.version = 1000;
		region.bus.address = "unexists";
		region.snapshot.address = "unexists";
		regions.push_back(region);
	}
	for(size_t i=0; i < service_count ; ++i) {
		Service svc;
		svc.name = newUUID();
		svc.endpoint.address = "unexists";
		services.push_back(svc);
	}
	for(size_t i=0; i < payload_count; ++i) {
		Payload pl;
		pl.id = newUUID();
		payloads.push_back(pl);
	}

	for(auto itreg = regions.begin(); itreg != regions.end(); ++itreg) {
		++counter.region_count;
		runner->push( std::bind(&RIPublisher::pubRegion,pub,*itreg) );
		for(auto itsvc = services.begin(); itsvc != services.end(); ++itsvc) {
			++counter.service_count;
			runner->push( std::bind(&RIPublisher::pubService,pub,itreg->id,itreg->version,*itsvc) );
		}
		for(auto itpld = payloads.begin(); itpld != payloads.end(); ++itpld ) {
			++counter.payload_count;
			runner->push( std::bind(&RIPublisher::pubPayload,pub,itreg->id,itreg->version,*itpld) );
		}
	}

	for(auto itreg = regions.begin(); itreg != regions.end(); ++itreg) {
		for(auto itsvc = services.begin(); itsvc != services.end(); ++itsvc) {
			++counter.rm_service_count;
			runner->push( std::bind(&RIPublisher::pubRemoveService,pub,itreg->id,itreg->version,itsvc->name) );
		}
		for(auto itpld = payloads.begin(); itpld != payloads.end(); ++itpld ) {
			++counter.rm_payload_count;
			runner->push( std::bind(&RIPublisher::pubRemovePayload,pub,itreg->id,itreg->version,itpld->id) );
		}
		++counter.rm_region_count;
		runner->push( std::bind(&RIPublisher::pubRemoveRegion,pub,itreg->id) );
	}

	return counter;
}



TEST(PubSub,Functional) {

	auto pub = std::make_shared<RIPublisher>(g_loop);
	auto sub = std::make_shared<RISubscriber>(g_loop);
	auto runner = std::make_shared<TaskRunner>(g_loop);
	auto ob = std::make_shared<MockObserver>();

	auto counter = make_pub_tasks(pub,runner,2,10,100);

	EXPECT_CALL(*ob,onRegion(testing::_)).Times(counter.region_count);
	EXPECT_CALL(*ob,onRmRegion(testing::_)).Times(counter.rm_region_count);
	EXPECT_CALL(*ob,onService(testing::_,testing::_,testing::_)).Times(counter.service_count);
	EXPECT_CALL(*ob,onRmService(testing::_,testing::_,testing::_)).Times(counter.rm_service_count);
	EXPECT_CALL(*ob,onPayload(testing::_,testing::_,testing::_)).Times(counter.payload_count);
	EXPECT_CALL(*ob,onRmPayload(testing::_,testing::_,testing::_)).Times(counter.rm_payload_count);

	ASSERT_EQ(0,pub->start(PUB_ADDRESS,true));
	ASSERT_EQ(0,sub->start(PUB_ADDRESS,ob));
	ASSERT_EQ(0,runner->start(1));

	while( runner->running() ) {
		if(0 == zloop_start(g_loop))
			break;
	}
}


