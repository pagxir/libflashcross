#ifndef _NPNETSTREAM_H_
#define _NPNETSTREAM_H_

class NPNetStream: public NPStream
{
	public:
		NPNetStream(NPP instance, const char *url, void *notifyData);

	private:
		size_t m_total;
		~NPNetStream();

	public:
		int Connect(struct sockaddr *si, size_t size);
		int PrepareGetURLHeaders(const char *refer);
		int PreparePostURLHeader(const char *url, const void *data, size_t len);

	public:
		NPP  m_instance;
		void OnRead(void);
		void OnWrite(void);
		void Close(bool notify);

	private:
		int ReadHeader(void);
		int RedirectURL(void);
		void ParseURLHeader(void);

	private:
		int CacheWrite(const void *buf, size_t len, size_t off);
		int m_lastError;
		int m_fildes;

	private:
		struct {
			char  *buf;
			size_t off, len;
		}out, in;

#define XYF_ENABLE   0x8000
#define XYF_SYNPUT   0x0001
#define XYF_SYNGET   0x0002

	private:
		int m_xyflags;
		char m_xy_snd[64];
		char m_xy_rcv[64];

	private:
		char *m_dstart;
		char  m_magic[32];
		FILE *m_cache_file;
        char *m_cache_name;
};

int CloseAllStream(NPP instance);
int plugin_NetworkNotify(int fd, int event);
int GetAddressByURL(const char *url, struct sockaddr_in *si);

#define WM_NETN (WM_USER + 0x9527)

#endif
