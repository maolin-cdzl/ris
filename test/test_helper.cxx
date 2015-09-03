#include "test_helper.h"


ReadableHelper::ReadableHelper(zloop_t* loop) :
	m_loop(loop),
	m_sock(nullptr),
	m_msg(nullptr)
{
}

ReadableHelper::~ReadableHelper() {
	if( m_msg ) {
		zmsg_destroy(&m_msg);
	}
	unregister();
}

int ReadableHelper::register_read(zsock_t* sock) {
	if( m_sock )
		return -1;

	int result = zloop_reader(m_loop,sock,&ReadableHelper::readAndGo,this);
	if( 0 == result ) {
		m_sock = sock;
	}
	return result;
}

int ReadableHelper::register_read_int(zsock_t* sock) {
	if( m_sock )
		return -1;

	int result = zloop_reader(m_loop,sock,&ReadableHelper::readAndInterrupt,this);
	if( 0 == result ) {
		m_sock = sock;
	}
	return result;
}

int ReadableHelper::unregister() {
	if( m_sock ) {
		zloop_reader_end(m_loop,m_sock);
		m_sock = nullptr;
		return 0;
	} else {
		return -1;
	}
}

int ReadableHelper::readAndGo(zloop_t* loop,zsock_t* reader,void* arg) {
	(void)loop;
	ReadableHelper* self = (ReadableHelper*)arg; 
	if( self->m_msg ) {
		zmsg_destroy(&self->m_msg);
	}

	self->m_msg = zmsg_recv(reader);
	return 0;
}

int ReadableHelper::readAndInterrupt(zloop_t* loop,zsock_t* reader,void* arg) {
	ReadableHelper* self = (ReadableHelper*)arg; 
	if( self->m_msg ) {
		zmsg_destroy(&self->m_msg);
	}

	self->m_msg = zmsg_recv(reader);
	zloop_reader_end(loop,reader);
	self->m_sock = nullptr;
	zsys_interrupted = 1;
	return 0;
}

