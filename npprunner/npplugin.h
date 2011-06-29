#ifndef _NPPLUGIN_H_
#define _NPPLUGIN_H_

class NPPlugin: public NPP_t
{
	public:
		~NPPlugin();

	public:
		int AttachWindow(void);
		int DetachWindow(void);
		int NPGeckoWindowNew(void);

	public:
		char * NewStreamName(char *buf, size_t len);
		int Resize(size_t width, size_t height);
		int SetWindow(void);

	private:
		int m_flags;
		NPWindow m_window;
		int m_major, m_minor;
};

#endif

