#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signalvar.h>

#include "rtld.h"

typedef int __pid_t;
typedef unsigned long int shmatt_t;

#define	LINUX_SIGTBLSZ			31
#define	LINUX_SIGEMPTYSET(set)		(set) = 0
#define	LINUX_SIGISMEMBER(set, sig)	(set) & _SIG_BIT(sig)
#define      LINUX_SIGADDSET(set, sig)   (set) |= _SIG_BIT(sig)

#define LINUX_SIGHUP            1
#define LINUX_SIGINT            2
#define LINUX_SIGQUIT           3
#define LINUX_SIGILL            4
#define LINUX_SIGTRAP           5
#define LINUX_SIGABRT           6
#define LINUX_SIGIOT            LINUX_SIGABRT
#define LINUX_SIGBUS            7
#define LINUX_SIGFPE            8
#define LINUX_SIGKILL           9
#define LINUX_SIGUSR1           10
#define LINUX_SIGSEGV           11
#define LINUX_SIGUSR2           12
#define LINUX_SIGPIPE           13
#define LINUX_SIGALRM           14
#define LINUX_SIGTERM           15
#define LINUX_SIGSTKFLT         16
#define LINUX_SIGCHLD           17
#define LINUX_SIGCONT           18
#define LINUX_SIGSTOP           19
#define LINUX_SIGTSTP           20
#define LINUX_SIGTTIN           21
#define LINUX_SIGTTOU           22
#define LINUX_SIGURG            23
#define LINUX_SIGXCPU           24
#define LINUX_SIGXFSZ           25
#define LINUX_SIGVTALRM         26
#define LINUX_SIGPROF           27
#define LINUX_SIGWINCH          28
#define LINUX_SIGIO         29
#define LINUX_SIGPOLL           LINUX_SIGIO
#define LINUX_SIGPWR            30
#define LINUX_SIGSYS            31

#define LINUX_SA_NOCLDSTOP      0x00000001
#define LINUX_SA_NOCLDWAIT      0x00000002
#define LINUX_SA_SIGINFO        0x00000004
#define LINUX_SA_RESTORER       0x04000000
#define LINUX_SA_ONSTACK        0x08000000
#define LINUX_SA_RESTART        0x10000000
#define LINUX_SA_INTERRUPT      0x20000000
#define LINUX_SA_NOMASK         0x40000000
#define LINUX_SA_ONESHOT        0x80000000

typedef struct __linux_sigset {
	uint32_t __val[1024 / 32];
} __linux_sigset_t;

struct linux_sigaction {
	void (* lsa_handler)(int);
	__linux_sigset_t lsa_mask;
	int lsa_flags;
	void * lsa_restorer;
};

static int linux_to_bsd_signal[LINUX_SIGTBLSZ] = {
	SIGHUP,  SIGINT,    SIGQUIT, SIGILL,
	SIGTRAP, SIGABRT,   SIGBUS,  SIGFPE,
	SIGKILL, SIGUSR1,   SIGSEGV, SIGUSR2,
	SIGPIPE, SIGALRM,   SIGTERM, SIGBUS,
	SIGCHLD, SIGCONT,   SIGSTOP, SIGTSTP,
	SIGTTIN, SIGTTOU,   SIGURG,  SIGXCPU,
	SIGXFSZ, SIGVTALRM, SIGPROF, SIGWINCH,
	SIGIO,   SIGURG,    SIGSYS
};

static int bsd_to_linux_signal[LINUX_SIGTBLSZ] = {
	LINUX_SIGHUP,  LINUX_SIGINT,    LINUX_SIGQUIT, LINUX_SIGILL,
	LINUX_SIGTRAP, LINUX_SIGABRT,   0,             LINUX_SIGFPE,
	LINUX_SIGKILL, LINUX_SIGBUS,    LINUX_SIGSEGV, LINUX_SIGSYS,
	LINUX_SIGPIPE, LINUX_SIGALRM,   LINUX_SIGTERM, LINUX_SIGURG,
	LINUX_SIGSTOP, LINUX_SIGTSTP,   LINUX_SIGCONT, LINUX_SIGCHLD,
	LINUX_SIGTTIN, LINUX_SIGTTOU,   LINUX_SIGIO,   LINUX_SIGXCPU,
	LINUX_SIGXFSZ, LINUX_SIGVTALRM, LINUX_SIGPROF, LINUX_SIGWINCH,
	0,             LINUX_SIGUSR1,   LINUX_SIGUSR2
};

static uint32_t bsd_to_linux_sigset(sigset_t * sa_mask,
		__linux_sigset_t * lmask)
{
	int b_sig, l_sig;
	uint32_t lsa_mask;

	LINUX_SIGEMPTYSET(lsa_mask);

	lsa_mask = sa_mask->__bits[0] & ~((1U << LINUX_SIGTBLSZ) - 1);

	for (b_sig = 1; b_sig <= LINUX_SIGTBLSZ; b_sig++) {
		if (SIGISMEMBER(*sa_mask, b_sig)) {
			if ((l_sig = bsd_to_linux_signal[_SIG_IDX(b_sig)]))
				LINUX_SIGADDSET(lsa_mask, l_sig);
		}
	}

	return 0;
}

static void linux_to_bsd_sigset(sigset_t * sa_mask,
		const __linux_sigset_t * lsa_mask)
{
	int b_sig, l_sig;

	SIGEMPTYSET(*sa_mask);

	sa_mask->__bits[0] = lsa_mask->__val[0] & ~((1U << LINUX_SIGTBLSZ) - 1);
	sa_mask->__bits[1] = 0;

	for (l_sig = 1; l_sig <= LINUX_SIGTBLSZ; l_sig++) {
		if (LINUX_SIGISMEMBER(lsa_mask->__val[0], l_sig)) {
			if ((b_sig = linux_to_bsd_signal[_SIG_IDX(l_sig)]))
				SIGADDSET(*sa_mask, b_sig);
		}
	}
}

int NSAPI(sigaction)(int l_sig, const struct linux_sigaction *lact,
		struct linux_sigaction *olact)
{
	int ret, sig = linux_to_bsd_signal[_SIG_IDX(l_sig)];
	struct sigaction act, oact;

	if (l_sig <= 0 || 32 < l_sig) {
		errno = EINVAL;
		ret = -1;
		return ret;
	}

	if (lact != NULL) {
		act.sa_handler = lact->lsa_handler;
		(void)linux_to_bsd_sigset(&act.sa_mask, &lact->lsa_mask);
		act.sa_flags = 0;

		if (lact->lsa_flags & LINUX_SA_NOCLDSTOP)
			act.sa_flags |= SA_NOCLDSTOP;

		if (lact->lsa_flags & LINUX_SA_NOCLDWAIT)
			act.sa_flags |= SA_NOCLDWAIT;

		if (lact->lsa_flags & LINUX_SA_SIGINFO)
			act.sa_flags |= SA_SIGINFO;

		if (lact->lsa_flags & LINUX_SA_ONSTACK)
			act.sa_flags |= SA_ONSTACK;

		if (lact->lsa_flags & LINUX_SA_RESTART)
			act.sa_flags |= SA_RESTART;

		if (lact->lsa_flags & LINUX_SA_ONESHOT)
			act.sa_flags |= SA_RESETHAND;

		if (lact->lsa_flags & LINUX_SA_NOMASK)
			act.sa_flags |= SA_NODEFER;
	}

	ret = sigaction(sig, (lact ? &act : NULL), &oact);

	if (olact == NULL)
		return ret;

	olact->lsa_handler  = oact.sa_handler;
	olact->lsa_restorer = NULL;
	bsd_to_linux_sigset(&oact.sa_mask, &olact->lsa_mask);
	olact->lsa_flags    = 0;

	if (oact.sa_flags & SA_NOCLDSTOP)
		olact->lsa_flags |= LINUX_SA_NOCLDSTOP;

	if (oact.sa_flags & SA_NOCLDWAIT)
		olact->lsa_flags |= LINUX_SA_NOCLDWAIT;

	if (oact.sa_flags & SA_SIGINFO)
		olact->lsa_flags |= LINUX_SA_SIGINFO;

	if (oact.sa_flags & SA_ONSTACK)
		olact->lsa_flags |= LINUX_SA_ONSTACK;

	if (oact.sa_flags & SA_RESTART)
		olact->lsa_flags |= LINUX_SA_RESTART;

	if (oact.sa_flags & SA_RESETHAND)
		olact->lsa_flags |= LINUX_SA_ONESHOT;

	if (oact.sa_flags & SA_NODEFER)
		olact->lsa_flags |= LINUX_SA_NOMASK;

	return ret;
}
