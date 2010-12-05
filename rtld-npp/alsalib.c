#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>

#include "rtld.h"

int NSAPI(snd_pcm_wait)(snd_pcm_t * pcm, int timeout)
{
	int delta;
	int retval = 0;
	struct timeval tini, tfin;
	snd_pcm_sframes_t delay = 0;

	gettimeofday(&tini, NULL);
	snd_pcm_delay(pcm, &delay);
	retval = snd_pcm_wait(pcm, timeout);

	gettimeofday(&tfin, NULL);
	delta = (tfin.tv_sec - tini.tv_sec) * 1000;
	delta = delta + (tfin.tv_usec - tini.tv_usec) / 1000;
	
	if (delay > timeout)
		delay = (timeout == -1)? delay: timeout;

	if (delay > delta)
		usleep(delay - delta);

	return retval;
}

