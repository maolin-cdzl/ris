#include "trackersession.h"
#include "ris/trackerapi.pb.h"
#include "zmqx/zprotobuf++.h"
#include "zmqx/zhelper.h"

TrackerSession::TrackerSession() :
	m_req(nullptr)
{
}

TrackerSession::~TrackerSession() {
	disconnect();
}

int TrackerSession::connect(const std::string& api_address,uint64_t timeout) {
	if( m_req )
		return -1;

	do {
		m_req = zsock_new(ZMQ_REQ);
		if( nullptr == m_req )
			break;
		if( -1 == zsock_connect(m_req,"%s",api_address.c_str()) )
			break;

		tracker::api::HandShake hs;
		if( -1 == zpb_send(m_req,hs) )
			break;
		
		if( zmq_wait_readable(m_req,timeout) <= 0 )
			break;

		if( -1 == zpb_recv(hs,m_req) )
			break;
		return 0;
	} while(0);

	if( m_req ) {
		zsock_destroy(&m_req);
	}
	return -1;
}

void TrackerSession::disconnect() {
	if( m_req ) {
		zsock_destroy(&m_req);
	}
}

int TrackerSession::getStatistics(RouteInfoStatistics* statistics) {
	assert( statistics );
	if( nullptr == m_req )
		return -1;

	do {
		tracker::api::StatisticsReq req;
		if( -1 == zpb_send(m_req,req)  )
			break;
		if( zmq_wait_readable(m_req,1000) <= 0 )
			break;
		tracker::api::StatisticsRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		statistics->region_size = rep.region_count();
		statistics->service_size = rep.service_count();
		statistics->payload_size = rep.payload_count();
		return 1;
	} while(0);

	return -1;
}

int TrackerSession::getRegion(RegionInfo* region,const std::string& uuid) {
	if( nullptr == m_req )
		return -1;

	do {
		tracker::api::RegionReq req;
		req.set_uuid(uuid);
		if( -1 == zpb_send(m_req,req)  )
			break;
		if( zmq_wait_readable(m_req,1000) <= 0 )
			break;
		tracker::api::RegionRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		if( rep.has_region() ) {
			region->uuid = rep.region().uuid();
			region->idc = rep.region().idc();
			region->bus_address = rep.region().bus_address();
			region->snapshot_address = rep.region().snapshot_address();
			region->version = rep.region().version();
			return 1;
		} else {
			region->uuid.clear();
			region->idc.clear();
			region->bus_address.clear();
			region->snapshot_address.clear();
			region->version = 0;
			return 0;
		}
	} while(0);

	return -1;
}

int TrackerSession::getServiceRouteInfo(RouteInfo* ri,const std::string& svc) {
	if( nullptr == m_req )
		return -1;
	do {
		tracker::api::ServiceRouteReq req;
		req.set_svc(svc);

		if( -1 == zpb_send(m_req,req) )
			break;
		if( zmq_wait_readable(m_req,1000) <= 0 )
			break;
		tracker::api::ServiceRouteRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		if( rep.has_route() ) {
			ri->target = rep.route().target();
			ri->region = rep.route().region();
			ri->address = rep.route().address();
			return 1;
		} else {
			ri->target.clear();
			ri->region.clear();
			ri->address.clear();
			return 0;
		}
	} while(0);
	return -1;
}

int TrackerSession::getPayloadRouteInfo(RouteInfo* ri,const std::string& payload) {
	if( nullptr == m_req )
		return -1;
	do {
		tracker::api::PayloadRouteReq req;
		req.set_payload(payload);

		if( -1 == zpb_send(m_req,req) )
			break;
		if( zmq_wait_readable(m_req,1000) <= 0 )
			break;
		tracker::api::PayloadRouteRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		if( rep.has_route() ) {
			ri->target = rep.route().target();
			ri->region = rep.route().region();
			ri->address = rep.route().address();
			return 1;
		} else {
			ri->target.clear();
			ri->region.clear();
			ri->address.clear();
			return 0;
		}
	} while(0);
	return -1;
}

int TrackerSession::getPayloadsRouteInfo(std::list<RouteInfo>& ris,const std::list<std::string>& payloads) {
	if( nullptr == m_req )
		return -1;
	do {
		tracker::api::PayloadsRouteReq req;
		for(auto it=payloads.begin(); it != payloads.end(); ++it) {
			req.add_payloads(*it);
		}

		if( -1 == zpb_send(m_req,req) )
			break;
		if( zmq_wait_readable(m_req,1000) <= 0 )
			break;
		tracker::api::PayloadsRouteRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		ris.clear();
		if( rep.routes_size() > 0 ) {
			for(size_t i=0; i < (size_t) rep.routes_size(); ++i) {
				RouteInfo ri;
				auto& route = rep.routes(i);
				ri.target = route.target();
				ri.region = route.region();
				ri.address = route.address();
				ris.push_back(ri);
			}
			return rep.routes_size();
		} else {
			return 0;
		}
	} while(0);
	return -1;
}


