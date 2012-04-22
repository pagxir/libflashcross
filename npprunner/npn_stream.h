#ifndef _NPNETSTREAM_H_
#define _NPNETSTREAM_H_

class socket_stream;
typedef struct waitcb *slotcb;
typedef void wait_call(void *data);

struct waitcb {
	int wt_magic;
	int wt_flags;
	int wt_count;
	struct waitcb *wt_next;
	struct waitcb **wt_prev;

	void *wt_data;
	size_t wt_value;

	void *wt_udata;
	wait_call *wt_callback;
};

struct chunkcb {
	int off;
	int len;
	char *buf;
};

class proto_stream
{
	public:
		virtual int block(void);
		virtual int connected(void);
		virtual int get_error(void);
		virtual int cantain_video(void);

	public:
		virtual int recv_data(void *buf, size_t len);
		int connect(const sockaddr_in *inaddrp, size_t len);

	public:
		int set_request(const char *header, const char *buf, size_t len);
		int set_request(const char *header);

	public:
		proto_stream(void);
		void rel(void);
		void ref(void);

	public:
		virtual int recv_wait(struct waitcb *waitp);
		virtual const char *response(void);
		virtual ~proto_stream();

	private:
		void on_read(void);
		void on_write(void);
		static void proto_read(void *upp);
		static void proto_write(void *upp);

	private:
		struct waitcb *m_waitp;
		struct chunkcb m_chunk;
		struct chunkcb m_header;

	private:
		int m_ref;
		int m_flags;
		struct waitcb m_read;
		struct waitcb m_write;
		socket_stream *m_stream;
};

class proto_stream_wrapper: public proto_stream
{
	public:
		int block(void) { return m_protop->block(); }
		int connected(void) { return m_protop->connected(); }
		int cantain_video(void) { return 0; }
		int get_error(void) { return m_protop->get_error(); }
		int recv_data(void *buf, size_t len);

	public:
		const char *response(void) { return m_protop->response(); }
		int recv_wait(struct waitcb *waitp) { return m_protop->recv_wait(waitp); }

	public:
		proto_stream_wrapper(proto_stream *proto);
		~proto_stream_wrapper();

	private:
		FILE *m_file;
		static int m_salt;
		proto_stream *m_protop;
};

class NPNetStream
{
	public:
		NPNetStream(NPP instance, void *notify,
				const char *url, proto_stream *protop);
		void startup(void);
		~NPNetStream();

	private:
		static void recv_wrapper(void *upp);
		struct waitcb m_read;
		int OnRead(void);

	private:
		char *m_url;
		char *m_url0;
		NPP  m_instance;
		NPStream m_stream;
		proto_stream *m_protop;

	private:
		int m_flags;
		size_t m_offset;
		char m_cache[64 * 1024];
};

#define NF_CREATED     0x8000
#define SF_CONNECTED   0x8000
#define SF_CLOSED      0x0800
#define PF_CREATED     0x8000
#define PF_VIDEO       0x4000
#define PF_ERROR       0x0800

int CloseAllStream(NPP instance);
int plugin_NetworkNotify(int fd, int event);

#define WM_NETN (WM_USER + 0x9527)

#endif
