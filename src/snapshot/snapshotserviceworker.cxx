#include <glog/logging.h>
#include "snapshot/snapshotserviceworker.h"
#include "zmqx/zprotobuf++.h"

SnapshotServiceWorker::SnapshotServiceWorker(const snapshot_package_t& snapshot) :
	m_snapshot(snapshot),
	m_tv_last(0)
{
}

SnapshotServiceWorker::~SnapshotServiceWorker() {
}


size_t SnapshotServiceWorker::sendItems(zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope,size_t count) {
	CHECK_NOTNULL(sock);
	CHECK(envelope);
	size_t c = 0;
	while( c < count && ! m_snapshot.empty() ) {
		auto p = m_snapshot.front();
		m_snapshot.pop_front();
		if( -1 == zpb_send(sock,envelope,*p) ) {
			int err = errno;
			LOG(ERROR) << "SnapshotServiceWorker send snapshot item error: " << err;
			return 0;
		}
		++c;
	}

	m_tv_last = ri_time_now();
	return m_snapshot.size();
}


