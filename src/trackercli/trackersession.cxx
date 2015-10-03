#include <glog/logging.h>
#include "trackercli/trackersession.h"
#include "ris/trackerapi.pb.h"
#include "zmqx/zprotobuf++.h"
#include "zmqx/zhelper.h"

TrackerSession::TrackerSession(uint64_t t) :
	m_req(nullptr),
	m_timeout(t)
{
}

TrackerSession::~TrackerSession() {
	disconnect();
}

int TrackerSession::connect(const std::string& api_address) {
	if( m_req )
		return -1;

	do {
		m_req = zsock_new(ZMQ_DEALER);
		CHECK_NOTNULL(m_req);
		if( -1 == zsock_connect(m_req,"%s",api_address.c_str()) ) {
			LOG(ERROR) << "Error when connect to: " << api_address;
			break;
		}

		tracker::api::HandShake hs;
		if( -1 == zpb_send(m_req,hs,true) ) {
			LOG(ERROR) << "Error when sending HandShake to tracker";
			break;
		}
		
		if( zmq_wait_readable(m_req,m_timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}

		if( -1 == zpb_recv(hs,m_req) ) {
			LOG(ERROR) << "Error when recv HandShake from req";
			break;
		}
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

std::shared_ptr<RouteInfoStatistics> TrackerSession::getStatistics() {

	do {
		if( nullptr == m_req )
			break;
		tracker::api::StatisticsReq req;
		if( -1 == zpb_send(m_req,req,true)  ) {
			LOG(ERROR) << "Error when send request to tracker";
			break;
		}
		if( zmq_wait_readable(m_req,m_timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}
		tracker::api::StatisticsRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		auto statistics = std::make_shared<RouteInfoStatistics>();
		for(auto it=rep.regions().begin(); it != rep.regions().end(); ++it) {
			statistics->regions.push_back(Region(*it));
		}
		for(auto it=rep.services().begin(); it != rep.services().end(); ++it) {
			statistics->services.push_back(*it);
		}
		statistics->payload_size = rep.payload_count();
		return std::move(statistics);
	} while(0);

	return nullptr;
}

std::shared_ptr<Region> TrackerSession::getRegion(const std::string& uuid) {
	do {
		if( nullptr == m_req )
			break;
		tracker::api::RegionReq req;
		req.set_uuid(uuid);
		if( -1 == zpb_send(m_req,req,true)  ) {
			LOG(ERROR) << "Error when send request to tracker";
			break;
		}
		if( zmq_wait_readable(m_req,m_timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}
		tracker::api::RegionRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		if( rep.has_region() ) {
			return std::make_shared<Region>(rep.region());
		}
	} while(0);

	return nullptr;
}

std::shared_ptr<RouteInfo> TrackerSession::getServiceRouteInfo(const std::string& svc) {
	do {
		if( nullptr == m_req )
			break;
		tracker::api::ServiceRouteReq req;
		req.set_svc(svc);

		if( -1 == zpb_send(m_req,req,true) ) {
			LOG(ERROR) << "Error when send request to tracker";
			break;
		}
		if( zmq_wait_readable(m_req,m_timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}
		tracker::api::ServiceRouteRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		if( rep.has_route() && svc == rep.route().target() ) {
			auto ri = std::make_shared<RouteInfo>();
			ri->target = rep.route().target();
			ri->endpoint.address = rep.route().endpoint().address();
			ri->endpoint.identity = rep.route().endpoint().identity();
			return std::move(ri);
		}
	} while(0);
	return nullptr;
}

std::shared_ptr<RouteInfo> TrackerSession::getPayloadRouteInfo(const std::string& payload) {
	do {
		if( nullptr == m_req )
			break;
		tracker::api::PayloadRouteReq req;
		req.set_payload(payload);

		if( -1 == zpb_send(m_req,req,true) ) {
			LOG(ERROR) << "Error when send request to tracker";
			break;
		}
		if( zmq_wait_readable(m_req,m_timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}
		tracker::api::PayloadRouteRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		if( rep.has_route() && payload == rep.route().target() ) {
			auto ri = std::make_shared<RouteInfo>();
			ri->target = rep.route().target();
			ri->endpoint.address = rep.route().endpoint().address();
			ri->endpoint.identity = rep.route().endpoint().identity();
			return std::move(ri);
		}
	} while(0);
	return nullptr;
}

std::list<RouteInfo> TrackerSession::getPayloadsRouteInfo(const std::list<std::string>& payloads) {
	std::list<RouteInfo> ris;
	do {
		if( nullptr == m_req )
			break;
		tracker::api::PayloadsRouteReq req;
		for(auto it=payloads.begin(); it != payloads.end(); ++it) {
			req.add_payloads(*it);
		}

		if( -1 == zpb_send(m_req,req,true) ) {
			LOG(ERROR) << "Error when send request to tracker";
			break;
		}
		if( zmq_wait_readable(m_req,m_timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}
		tracker::api::PayloadsRouteRep rep;
		if( -1 == zpb_recv(rep,m_req) )
			break;

		for(auto it=rep.routes().begin(); it != rep.routes().end(); ++it) {
			RouteInfo ri;
			ri.target = it->target();
			if( it->has_endpoint() ) {
				ri.endpoint.address = it->endpoint().address();
				ri.endpoint.identity = it->endpoint().identity();
			}
			ris.push_back(ri);
		}
	} while(0);
	return std::move(ris);
}


