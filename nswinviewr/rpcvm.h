#ifndef _RPCVM_H_
#define _RPCVM_H_

#include <stdint.h>

class rpccall
{
	public:
		int result(int fd, uint32_t seq);
		rpccall(char *buf, size_t len);
		~rpccall();

	public:
		char *pop8lz(void);
		void *pop32p(void);
		void *pop8ll(size_t len);
		uint32_t pop32u(void);
		int32_t  pop32i(void);
		uint16_t pop16u(void);
		int16_t  pop16i(void);
		uint8_t pop8u(void);
		int8_t  pop8i(void);

	public:
		int push32u(uint32_t uval);
		int push32p(void *refter);
		int push16u(uint16_t uval);

	public:
		char *m_outbase;
		size_t m_outlen, m_outoff;

	private:
		char *m_buf;
		size_t m_len, m_off;
};

int rpc_dispatch(int rpcfd, char *buf, size_t len);
int rpc_call(int call, rpccall & rpc);

#define RPC_f(f)              rpc_##f

enum {
	RPC_f(Error)             =  -1, 
	RPC_f(Result)            =   0,
	RPC_f(NPP_New)           =   1,
	RPC_f(NPP_SetWindow)     =   2,
	RPC_f(NPP_Destroy)       =   3,
	RPC_f(NPP_GetValue)      =   4,
	RPC_f(NPP_NewStream)     =   5,
	RPC_f(NPP_WriteReady)    =   6,
	RPC_f(NPP_Write)         =   7,
	RPC_f(NPP_StreamAsFile)  =   8,
	RPC_f(NPP_DestroyStream) =   9,
	RPC_f(NPP_Print)         =  10,
	RPC_f(NPP_URLNotify)     =  11,
	RPC_f(NPP_SetValue)      =  12,
	RPC_f(NPP_HandleEvent)   =  13
};

#endif
