#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <winsock.h>
#include <set>

#include "rpcvm.h"
#include "npupp.h"
#include "nputils.h"
#include "npplugin.h"
#include "npnetstream.h"

#define RPCSIZE (64 * 1024)

extern std::set<NPP> __plugin_instance_list;

char * rpccall::pop8lz(void)
{
	char *p = m_buf + m_off;
	m_off += strlen(p);
	m_off ++;
	assert (m_off < m_len);
	return p;
}

void * rpccall::pop32p(void)
{
	void *vp = 0;
	assert (m_off + 4 < m_len);
	memcpy(&vp, m_buf + m_off, 4);
	m_off += 4;
	return vp;
}

uint16_t rpccall::pop16u(void)
{
	uint16_t uval;
	assert (m_off + 2 < m_len);
	memcpy(&uval, m_buf + m_off, 2);
	m_off += 2;
	return uval;
}

int32_t rpccall::pop32i(void)
{
	int32_t ival;
	assert (m_off + 4 < m_len);
	memcpy(&ival, m_buf + m_off, 4);
	m_off += 4;
	return ival;
}

uint32_t rpccall::pop32u(void)
{
	uint32_t uval;
	assert (m_off + 4 < m_len);
	memcpy(&uval, m_buf + m_off, 4);
	m_off += 4;
	return uval;
}

uint8_t rpccall::pop8u(void)
{
	assert (m_off + 1 < m_len);
	return (uint8_t)m_buf[m_off++];
}

int16_t rpccall::pop16i(void)
{
	int16_t ival;
	assert (m_off + 2 < m_len);
	memcpy(&ival, m_buf + m_off, 2);
	m_off += 2;
	return ival;
}

rpccall::rpccall(char * buf, size_t len)
{
	m_off = 0;
	m_len = len;
	m_buf = buf;

	m_outoff = 12;
	m_outlen = (1024 * 160);
	m_outbase = new char[m_outlen];
}

int rpccall::push32p(void *refer)
{
	assert (m_outoff + 4 < m_outlen);
	memcpy(m_outbase + m_outoff, &refer, 4);
	m_outoff += 4;
	return 0;
}

int rpccall::push32u(uint32_t uval)
{
	assert (m_outoff + 4 < m_outlen);
	memcpy(m_outbase + m_outoff, &uval, 4);
	m_outoff += 4;
	return 0;
}

int rpccall::push16u(uint16_t uval)
{
	assert (m_outoff + 2 < m_outlen);
	memcpy(m_outbase + m_outoff, &uval, 2);
	m_outoff += 2;
	return 0;
}

void *rpccall::pop8ll(size_t len)
{
	char *buf = m_buf + m_off;
	assert (m_off + len < m_len);
	m_off += len;
	return buf;
}

rpccall::~rpccall(void)
{

}

int rpccall::result(int fd, uint32_t seq)
{
	int error;
	uint32_t *prefix = (uint32_t*)m_outbase;
	m_outbase[m_outoff++] = 0;
	prefix[0] = m_outoff;
	prefix[1] = 0;
	prefix[2] = seq;
	error = send(fd, m_outbase, m_outoff, 0);
	printf("rpccall::result: %d\n", error);
	return 0;
}

static char pktbuf[163840];
static size_t total = 0;

int rpc_dispatch(int fd, char *buf, size_t len)
{
	int off;
	uint32_t *prefix = (uint32_t*)pktbuf;
	assert (total + len < sizeof pktbuf);
	memcpy(pktbuf + total, buf, len);
	total += len;
	printf("rpc_dispatch: %d\n", len);

	static int callref = 0;
	assert (callref == 0);
	callref ++;
	for ( ; ; ) {
		off = prefix[0];
		assert (total < 4 || off < sizeof pktbuf);

		if (total < 4 || total < off) {
			callref -- ;
			return 0;
		}

#if 0
		printf("Full Packet Receive: %d %d %p!\n",
				off, prefix[1], prefix[2]);
#endif
		rpccall rpc((char*)(&prefix[3]), off - 12);
		rpc_call(prefix[1], rpc);
		rpc.result(fd, prefix[2]);
		total -= off;
		memmove(pktbuf, pktbuf + off, total);
	}
	callref --;
	return 0;
}

const char *rpc_name(int call)
{
#define RPC_s(n) \
	if (call == rpc_##n) return #n;
	RPC_s(Error);
	RPC_s(Result);
	RPC_s(NPP_New);
	RPC_s(NPP_SetWindow);
	RPC_s(NPP_Destroy);
	RPC_s(NPP_GetValue);
	RPC_s(NPP_NewStream);
	RPC_s(NPP_WriteReady);
	RPC_s(NPP_Write);
	RPC_s(NPP_StreamAsFile);
	RPC_s(NPP_DestroyStream);
	RPC_s(NPP_Print);
	RPC_s(NPP_URLNotify);
	RPC_s(NPP_SetValue);
	RPC_s(NPP_HandleEvent);
#undef RPC_s
	return "Unkown Name";
}

int rpc_call(int call, rpccall & rpc)
{
	uint16_t mode;
	uint16_t count;
	uint16_t stype;
	char *minetype;

	NPBool seekable;
	NPWindow *win;
	NPStream *stream;
	NPPlugin *instance;
	NPSavedData *saved;

	NPError error;

	void *  buffer;
	int32_t offset, length;
	const char ** argv, ** argn;

	switch (call) {
		case RPC_f(NPP_New):
			instance = new NPPlugin(NULL);
			minetype = (char*)rpc.pop8lz();
			printf("type: %s\n", minetype);
			instance->ndata = rpc.pop32p();
			mode  = rpc.pop16u();
			count = rpc.pop16i();
			argn = new const char * [count];
			argv = new const char * [count];
			for (int i = 0; i < count; i++) {
				argn[i] = rpc.pop8lz();
				argv[i] = rpc.pop8lz();
				printf("%s = %s\n", argn[i], argv[i]);
				argv[i] = (argv[i][0]?argv[i]:NULL);
			}
			saved = (NPSavedData*)rpc.pop32p();
			error = __pluginfunc.newp(minetype,
					instance, mode, count, (char **)argn, (char **)argv, NULL);
			rpc.push32u(error);
			delete[] argn;
			delete[] argv;
			if (error != NPERR_NO_ERROR) {
				delete instance;
				break;
			}
			__plugin_instance_list.insert(instance);
			rpc.push32p(instance);
			printf("NPP create finish!\n");
			break;
		case RPC_f(NPP_Destroy):
			instance = (NPPlugin*)rpc.pop32p();
			saved = (NPSavedData*)rpc.pop32p();
			CloseAllStream(instance);
			error = __pluginfunc.destroy(instance, NULL);
			__plugin_instance_list.erase(instance);
			delete instance;
			rpc.push32u(error);
			break;
		case RPC_f(NPP_NewStream):
			instance = (NPPlugin*)rpc.pop32p();
			minetype = rpc.pop8lz();
			stream = new NPStream;
			stream->ndata = rpc.pop32p();
			stream->url   = strdup(rpc.pop8lz());
			stream->end   = rpc.pop32u(); 
			stream->lastmodified   = rpc.pop32u(); 
			stream->notifyData     = rpc.pop32p(); 
			stream->headers        = strdup(rpc.pop8lz());
			seekable = rpc.pop8u();
			error = __pluginfunc.newstream(instance, minetype, stream,
					seekable, &stype);
			printf("----------------------\n");
			printf(": %s\n\n%s\n", stream->url, stream->headers);
			printf("----------------------\n");
			rpc.push32u(error);
			if (error != NPERR_NO_ERROR) {
				free((void*)stream->headers);
				free((void*)stream->url);
				delete stream;
				break;
			}
			rpc.push32p(stream);
			rpc.push16u(stype);
			break;
		case RPC_f(NPP_WriteReady):
			instance = (NPPlugin*)rpc.pop32p();
			stream = (NPStream*)rpc.pop32p();
			length = __pluginfunc.writeready(instance, stream);
			rpc.push32u(length);
			break;
		case RPC_f(NPP_Write):
			instance = (NPPlugin*)rpc.pop32p();
			stream = (NPStream*)rpc.pop32p();
			offset = rpc.pop32i();
			length = rpc.pop32i();
			buffer = rpc.pop8ll(length);
			length = __pluginfunc.write(instance, stream, offset,
					length, buffer);
			rpc.push32u(length);
			break;
		case RPC_f(NPP_DestroyStream):
			instance = (NPPlugin*)rpc.pop32p();
			stream   = (NPStream*)rpc.pop32p();
			error    = rpc.pop32u();
			error    = __pluginfunc.destroystream(instance, stream, error);
			rpc.push32u(error);
			free((void*)stream->headers);
			free((void*)stream->url);
			delete stream;
			break;
		case rpc_NPP_SetWindow:
			instance = (NPPlugin*)rpc.pop32p();
			win = (NPWindow*)rpc.pop32p();
			error = instance->SetWindow();
			rpc.push32u(error);
			break;
		case rpc_NPP_GetValue:
			printf("NPP_GetValue\n");
			assert(0);
			break;
		default:
			printf("Unkown function call: %s!\n", rpc_name(call));
			assert(0);
			break;
	}
	return 0;
}

