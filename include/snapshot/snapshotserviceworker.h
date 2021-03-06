#pragma once

#include <functional>
#include <czmq.h>
#include "snapshot/snapshotable.h"
#include "zmqx/zlooptimer.h"
#include "zmqx/zenvelope.h"
#include "ris/ritypes.h"
#include "ris/snapshot.pb.h"

class SnapshotServiceWorker {
public:
	SnapshotServiceWorker(const snapshot_package_t& snapshot);
	~SnapshotServiceWorker();

	size_t sendItems(zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelop,size_t count);

	inline ri_time_t lastSend() const {
		return m_tv_last;
	}
private:
	snapshot_package_t						m_snapshot;
	ri_time_t								m_tv_last;
};

