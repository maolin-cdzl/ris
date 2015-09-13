#include "snapshot/snapshotfeature.h"


SnapshotFeature::SnapshotFeature(zloop_t* loop) :
	m_loop(loop),
	m_frontend(nullptr),
	m_backend(nullptr),
	m_backreader(loop)
{
}

SnapshotFeature::~SnapshotFeature() {
	stop();
}

snapshot_package_t SnapshotFeature::buildSnapshot() {
	assert( m_frontend );

	zsock_signal(m_frontend,0);
	zsock_wait(m_frontend);
	return std::move(m_package);
}

int SnapshotFeature::start(const std::shared_ptr<ISnapshotable>& impl) {
	if( m_backreader.isActive() )
		return -1;

	do {
		m_frontend = zsys_create_pipe(&m_backend);
		if( nullptr == m_frontend || nullptr == m_backend )
			break;
		if( -1 == m_backreader.start(m_backend,std::bind<int>(&SnapshotFeature::onRequest,this,std::placeholders::_1)) ) {
			break;
		}
		m_impl = impl;
		return 0;
	} while(0);

	stop();
	return -1;
}

void SnapshotFeature::stop() {
	if( m_backreader.isActive() ) {
		m_backreader.stop();
	}
	if( m_frontend ) {
		zsock_destroy(&m_frontend);
	}
	if( m_backend ) {
		zsock_destroy(&m_backend);
	}
	m_impl.reset();
	m_package.clear();
}

int SnapshotFeature::onRequest(zsock_t* sock) {
	zsock_wait(sock);
	m_package = m_impl->buildSnapshot();
	zsock_signal(sock,0);
	return 0;
}

