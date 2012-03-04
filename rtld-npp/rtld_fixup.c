#include <math.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <pthread_np.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/signalvar.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "rtld.h"

#ifndef ENABLE_DEBUG
#define dbg_trace(fmt, args...) (void *)0
#else
#define dbg_trace(fmt, args...) printf(fmt, ##args)
#endif

#define WARN(exp, msg) if (exp); else dbg_trace("%s\n", msg)

typedef int dl_callback(
		struct dl_phdr_info * info, size_t size, void * data);

typedef int __pid_t;
typedef unsigned long int shmatt_t;

static int init_done = 0;
static pthread_mutexattr_t mutexattr_fixobj;

void fixup_init(void)
{
	if (init_done == 0) {
		pthread_mutexattr_init(&mutexattr_fixobj);
		pthread_mutexattr_settype(&mutexattr_fixobj, PTHREAD_MUTEX_RECURSIVE);
	}
	init_done = 1;
}

struct linux_ipc_perm
{
	__key_t key;
	__uid_t uid;
	__gid_t gid;
	__uid_t cuid;
	__gid_t cgid;
	unsigned short int mode;	
	unsigned short int __pad1;
	unsigned short int seq;	
	unsigned short int __pad2;
	unsigned long int __unused1;
	unsigned long int __unused2;
};

struct linux_shmid_ds
{
	struct linux_ipc_perm shm_perm;		
	size_t shm_segsz;			
	__time_t shm_atime;			
	unsigned long int __unused1;
	__time_t shm_dtime;			
	unsigned long int __unused2;
	__time_t shm_ctime;			
	unsigned long int __unused3;
	__pid_t shm_cpid;			
	__pid_t shm_lpid;			
	shmatt_t shm_nattch;		
	unsigned long int __unused4;
	unsigned long int __unused5;
};

static void linux_shmid_ds_copyin(
		struct linux_shmid_ds * l_ds, struct shmid_ds * buf)
{
#define XX(field) buf->shm_perm.field = l_ds->shm_perm.field
#define YY(field) buf->field = l_ds->field
	XX(key);
	XX(uid);
	XX(cuid);
	XX(cgid);
	XX(mode);
	XX(seq);
	YY(shm_segsz);
	YY(shm_lpid);
	YY(shm_cpid);
	YY(shm_nattch);
	YY(shm_atime);
	YY(shm_dtime);
	YY(shm_ctime);
#undef XX
#undef YY
}

static void linux_shmid_ds_copyout(
		struct linux_shmid_ds * buf, struct shmid_ds * l_ds)
{
#define XX(field) buf->shm_perm.field = l_ds->shm_perm.field
#define YY(field) buf->field = l_ds->field
	XX(key);
	XX(uid);
	XX(cuid);
	XX(cgid);
	XX(mode);
	XX(seq);
	YY(shm_segsz);
	YY(shm_lpid);
	YY(shm_cpid);
	YY(shm_nattch);
	YY(shm_atime);
	YY(shm_dtime);
	YY(shm_ctime);
#undef XX
#undef YY
}

int NSAPI(dl_iterate_phdr)(dl_callback * callback, void * data)
{
	dbg_trace("dl_iterate_phdr: callback %p, data %p\n", callback, data);
	return -1;
}

int NSAPI(shmctl)(int shmid, int cmd, struct linux_shmid_ds * buf)
{
	int error;
	struct shmid_ds l_ds;
	struct shmid_ds *pds = (struct shmid_ds *)buf;

	switch(cmd) {
		case IPC_SET:
		case IPC_RMID:
			if (pds == NULL)
				break;
			linux_shmid_ds_copyin(buf, &l_ds);
			pds = &l_ds;
			break;

		case IPC_STAT:
			if (pds != NULL)
				pds = &l_ds;
			break;
	}

	error = shmctl(shmid, cmd, pds);

	if (error != 0) {
		dbg_trace("shmctl: shmid %d, cmd %d, buf %p, error %s\n",
				shmid, cmd, buf, strerror(errno));
	} else if (cmd == IPC_STAT) {
		if (buf != NULL)
			linux_shmid_ds_copyout(buf, &l_ds);
	}

	return error;
}

int NSAPI(shmget)(key_t key, size_t size, int flag)
{
	int error;
	error = shmget(key, size, flag);
	if (error != -1)
		return error;
	dbg_trace("shmget: key %p, size %ld, flag %d, error %s\n",
			key, size, flag, strerror(errno));
	return error;
}

void * NSAPI(shmat)(int shmid, const void * addr, int flag)
{
	void * retval;
	dbg_trace("shmat: shmid %d, addr %p, flag %d\n",
			shmid, addr, flag);
	retval = shmat(shmid, addr, flag);
	if (retval != MAP_FAILED)
		return retval;

	dbg_trace("shmat: shmid %d, addr %p, flag %d, error %s\n",
			shmid, addr, flag, strerror(errno));
	abort();
	return retval;
}

struct sockaddr_in_gnu {
	unsigned short sin_family;
	unsigned short sin_port;
	unsigned int   sin_addr;
};

int NSAPI(connect)(int fd, const struct sockaddr * addr, size_t addrlen)
{
	struct sockaddr_in in_gnu;
	struct sockaddr_in_gnu * compat;

	if (addr == NULL || addrlen != 16); {
		errno = EINVAL;
		return -1;
	}

	memcpy(&in_gnu, addr, sizeof(in_gnu));
	dbg_trace("connect family %d, port %d,  addr %x\n",
			in_gnu.sin_family, htons(in_gnu.sin_port), (in_gnu.sin_addr));

	compat = (struct sockaddr_in_gnu *)addr;

	if (in_gnu.sin_family != AF_INET &&
			compat->sin_family == AF_INET) {
		in_gnu.sin_family = compat->sin_family;
		in_gnu.sin_port   = compat->sin_port;
		in_gnu.sin_addr.s_addr = compat->sin_addr;
		return connect(fd, (struct sockaddr *)&in_gnu, sizeof(in_gnu));
	}

	return connect(fd, addr, addrlen);
}

static inline void swap(void  ** a, void ** b)
{
	void * tt;
	tt = *a;
	*a = *b;
	*b = tt;
}

int NSAPI(getaddrinfo)(const char * hostname, const char * servname,
		const struct addrinfo * hints, struct addrinfo ** res)
{
	int error;
	struct addrinfo * info;

	error = getaddrinfo(hostname, servname, hints, res);
	if (error != 0)
		return error;

	for (info = *res; info; info = info->ai_next)
		swap((void **)&info->ai_canonname,
				(void **)&info->ai_addr);
	return error;
}

void NSAPI(freeaddrinfo)(struct addrinfo * ai)
{
	struct addrinfo * info;
	if (ai == NULL)
		return;

	for (info = ai; info; info = info->ai_next)
		swap((void **)&info->ai_canonname,
				(void **)&info->ai_addr);
	freeaddrinfo(ai);
}

struct utsname_gnu
{
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

int NSAPI(uname)(struct utsname_gnu * name)
{
	int error = 0;

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}
#define XX(f, n) strncpy(name->f, n, 64);
	XX(sysname, "Linux");
	XX(nodename, "myhost");
	XX(release, "2.6.33-ARCH");
	XX(version, "#1 SMP PREEMPT Thu May 13 12:06:25 CEST 2010");
	XX(machine, "i386");
#undef XX
	//name->domainname[0] = 0;
	return error;
}

char * NSAPI(setlocale)(int category, const char * locale)
{
	if (category == 5)
		return setlocale(LC_MESSAGES, locale);

	return setlocale(category, locale);
}

struct dirent_gnu {
	size_t d_ino;
	size_t d_off;
	unsigned short int d_reclen;
	unsigned char d_type;
	char d_name[256];
};

struct dirent_gnu * NSAPI(readdir)(DIR * dirp)
{
	struct dirent * dir;
	static __thread struct dirent_gnu dir_gnu;

	dir = readdir(dirp);
	if (dir == NULL)
		return NULL;

	dir_gnu.d_ino = dir->d_fileno;
	dir_gnu.d_off = 0;
	dir_gnu.d_reclen = dir->d_reclen;
	dir_gnu.d_type = dir->d_type;
	strncpy(dir_gnu.d_name, dir->d_name, 256);

	return &dir_gnu;
}

int NSAPI(semop)(int semid, struct sembuf * array, size_t nops)
{
	int error;

	error = semop(semid, array, nops);
	if (error == 0)
		return error;

	dbg_trace("semop: semid %d, array %p, nops %ld, error %s\n",
			semid, array, nops, strerror(errno));

	return error;
}

int NSAPI(semget)(key_t key, int nsems, int flag)
{
	int error;
	error = semget(key, nsems, flag);
	if (error != -1)
		return error;

	dbg_trace("semget: key %p, nsems %d, flag %d, error %s\n",
			key, nsems, flag, strerror(errno));
	return error;
}

int NSAPI(semctl)(int semid, int semnum, int cmd, void * opt)
{
	int error;

	switch (cmd) {
		case 11:
			cmd = GETPID;
			break;
		case 12:
			cmd = GETVAL;
			break;
		case 13:
			cmd = GETALL;
			break;
		case 14:
			cmd = GETNCNT;
			break;
		case 15:
			cmd = GETZCNT;
			break;
		case 16:
			cmd = SETVAL;
			break;
		case 17:
			cmd = SETALL;
			break;
		case 18:
			cmd = SEM_STAT;
			break;
		case 19:
			cmd = SEM_INFO;
			break;
		case IPC_STAT:
		case IPC_SET:
		case IPC_RMID:
		default:
			dbg_trace("[semctl] %d %d %d\n", semid, semnum, cmd);
			exit(0);
			break;
	}

	error = semctl(semid, semnum, cmd, opt);
	if (error != -1)
		return error;

	dbg_trace("semctl: semid %d, semnum %d, smd %d, error %s\n",
			semid, semnum, cmd, strerror(errno));
	return error;
}

struct sysinfo_gnu {
	long uptime;
	unsigned long loads[3];
	unsigned long totalram;
	unsigned long freeram;
	unsigned long sharedram;
	unsigned long bufferram;
	unsigned long totalswap;
	unsigned long freeswap;
	unsigned short procs;
	unsigned short pad;
	unsigned long totalhigh;
	unsigned long freehigh;
	unsigned int mem_unit;
	char _f[20 - 2 * sizeof(long) - sizeof(int)];
};

int NSAPI(sysinfo)(struct sysinfo_gnu * info)
{
	if (info == NULL) {
		errno = EINVAL;
		return -1;
	}
	dbg_trace("sysinfo fixup call\n");
	info->uptime = 7433;
	info->loads[0] = 0;
	info->loads[1] = 1120;
	info->loads[2] = 0;
	info->totalram = 1516548096;
	info->freeram = 1002897408;
	info->sharedram = 0;
	info->bufferram = 270336;
	info->totalswap = 1024 * 1024 * 1024;
	info->freeswap  = 1024 * 1024 * 1024;
	info->procs = 217;
	info->freehigh = 168878080;
	info->totalhigh = 611655680;
	return 0;
}

void *FPX_Init(void *ptr);
static int dummy_curllib = 0;
static int dummy_flashsupport = 0;

void * NSAPI(dlopen)(const char * path, int mode)
{
	ssize_t len;
	void *handle;
	char *p, buf[1024];
	static int glfix = 0;

	dbg_trace("dlopen %s %x\n", path, mode);
	if ((mode & 0x4) != 0) {
		dbg_trace("bug fixed for mode operation.\n");
		mode = (mode & ~0x04) | RTLD_NOLOAD;
	}

	if (strstr(path, "libGL.so") &&
		(glfix == 0) && (mode & RTLD_NOLOAD)) {
		dbg_trace("bug fixed for loading libGL.\n");
		return (glfix++ == 0? NULL: NSAPI(dlopen)(path, mode));
	}

	if (strstr(path, "libflashsupport") != NULL) {
		dummy_flashsupport++;
		return &dummy_flashsupport;
	}

	handle = dlopen(path, mode);
	if (handle != NULL) {
	    dbg_trace("dlopen %s %x %p\n", path, mode, handle);
		return handle;
	}

	len = strstr(path, ".so") - path;
	if (len > 0 && path[len + 3] != 0) {
		dbg_trace("dlopen fixup %s %x %p\n", path, mode, handle);
		strlcpy(buf, path, len + 4);
		handle = dlopen(buf, mode);
	}

	if (handle == NULL) {
		fprintf(stderr, "[fixme] please install missing package.\n");
		fprintf(stderr, "[fixme] dlopen %s failure.\n", path);
	}

	return handle;
}

void * NSAPI(dlsym)(void * handle, const char * name)
{
	void *retp;
	dbg_trace("dlsym: %s, handle %p\n", name, handle);

	if (handle == &dummy_flashsupport) {
		if (strcmp("FPX_Init", name))
			return NULL;
		return (void *)FPX_Init;
	}

	retp = dlfunc(handle, name);
	if (retp != NULL)
		return retp;

	retp = dlsym(handle, name);
	if (retp != NULL)
		return retp;

	dbg_trace("fixup dlsym %s fail\n", name);
	return NULL;
}

int NSAPI(dlclose)(void * handle)
{
	printf("dlclose: %p\n", handle);
	if (handle == &dummy_flashsupport) {
		WARN(dummy_flashsupport > 0, "dummy_flashsupport");
		dummy_flashsupport--;
		return 0;
	}

	return dlclose(handle);
}

long NSAPI(sysconf)(int name)
{
	if (name == 30)
		return sysconf(_SC_PAGESIZE);

	dbg_trace("sysconf %d\n", name);
	errno = EINVAL;
	return -1;
}

static const int __map_prot[] = {
	PROT_NONE, PROT_READ, PROT_WRITE, PROT_READ| PROT_WRITE,
	PROT_EXEC, PROT_READ| PROT_EXEC, PROT_WRITE| PROT_EXEC,
	PROT_EXEC| PROT_WRITE| PROT_READ
};

int NSAPI(mprotect)(void * addr, size_t len, int prot)
{
	if (prot & 0xfffffff8) {
		dbg_trace("[mprotect] bad prot: 0x%x\n", prot); 
		errno = EINVAL;
		return -1;
	}

	return mprotect(addr, len, __map_prot[prot & 0x7]);
}

void * NSAPI(mmap)(void * addr, size_t len, int prot,
		int flags, int fd, size_t offset)
{
	int flags_bsd, prot_bsd;

	if (prot & 0xfffffff8) {
		dbg_trace("[mmap] bad prot: 0x%x\n", prot); 
		errno = EINVAL;
		return MAP_FAILED;
	}

	prot_bsd = __map_prot[prot & 0x7];

	switch (flags & 0xF) {
		case 0x01:
			flags_bsd = MAP_SHARED;
			break;
		case 0x02:
			flags_bsd = MAP_PRIVATE;
			break;
		default:
			dbg_trace("[mmap] bad flags: 0x%x\n", flags); 
			errno = EINVAL;
			return MAP_FAILED;
	}

	if (flags & 0xffffffC0) {
		dbg_trace("[mmap] bad flags: 0x%x\n", flags); 
		errno = EINVAL;
		return MAP_FAILED;
	}

#define XX(result, flags, linux, native) \
	do { \
		if (flags & linux) \
		result |= native; \
	} while ( 0 )

	XX(flags_bsd, flags, 0x10, MAP_FIXED);
	XX(flags_bsd, flags, 0x20, MAP_ANON);
#undef XX

	return mmap(addr, len, prot_bsd, flags_bsd, fd, offset);
}

struct stat64_gnu {
	unsigned long long	st_dev;
	unsigned char	__pad0[4];
	unsigned long	__st_ino;
	unsigned int	st_mode;
	unsigned int	st_nlink;
	unsigned long	st_uid;
	unsigned long	st_gid;
	unsigned long long	st_rdev;
	unsigned char	__pad3[4];
	long long	st_size;
	unsigned long	st_blksize;
	unsigned long long	st_blocks;
	struct timespec st_atim0;
	struct timespec st_mtim0;
	struct timespec st_ctim0;
	unsigned long long st_ino;
};

static inline void st64copy(struct stat64_gnu * st_gnu,
		const struct stat * st_bsd)
{
#define XX(field) \
	st_gnu->field = st_bsd->field;

	XX(st_dev);
	XX(st_nlink);
	XX(st_uid);
	XX(st_gid);
	XX(st_rdev);
	XX(st_size);
	XX(st_blksize);
	XX(st_blocks);
	XX(st_ino);

#undef XX

	st_gnu->st_mode = (st_bsd->st_mode);
	st_gnu->__st_ino = st_bsd->st_ino;
#if 1
	st_gnu->st_atim0 = st_bsd->st_atimespec;
	st_gnu->st_mtim0 = st_bsd->st_mtimespec;
	st_gnu->st_ctim0 = st_bsd->st_ctimespec;
#endif
}

int NSAPI(__xstat64)(int ver, const char * path, struct stat64_gnu * stat_buf)
{
	int error;
	struct stat st_bsd;

	error = stat(path, stat_buf? &st_bsd: NULL);
	if (error != 0 || stat_buf == NULL)
		return error;

	st64copy(stat_buf, &st_bsd);
	return error;
}

int NSAPI(__lxstat64)(int ver, const char * path, struct stat64_gnu * stat_buf)
{
	int error;
	struct stat st_bsd;

	error = lstat(path, stat_buf? &st_bsd: NULL);
	if (error != 0 || stat_buf == NULL)
		return error;

	st64copy(stat_buf, &st_bsd);

	return error;
}

struct stat_gnu {
	unsigned long long int st_dev;
	unsigned short int __pad1;
	unsigned long int st_ino;
	unsigned int st_mode;
	unsigned int st_nlink;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long long int st_rdev;
	unsigned short int __pad2;
	long int st_size;
	long int st_blksize;
	long int st_blocks;
	struct timespec st_atim0;
	struct timespec st_mtim0;
	struct timespec st_ctim0;
	unsigned long int __unused4;
	unsigned long int __unused5;
};

static inline void stcopy(struct stat_gnu * st_gnu, const struct stat * st_bsd)
{
#define XX(field) st_gnu->field = st_bsd->field;
	XX(st_dev);
	XX(st_ino);
	XX(st_mode);
	XX(st_nlink);
	XX(st_uid);
	XX(st_gid);
	XX(st_rdev);
	XX(st_size);
	XX(st_blksize);
	XX(st_blocks);
#undef XX
#if 1
	st_gnu->st_mode = (st_bsd->st_mode);
	st_gnu->st_atim0 = st_bsd->st_atimespec;
	st_gnu->st_mtim0 = st_bsd->st_mtimespec;
	st_gnu->st_ctim0 = st_bsd->st_ctimespec;
#endif
}

int NSAPI(__xstat)(int ver, const char * path, struct stat_gnu * stat_buf)
{
	int error;
	struct stat st_bsd;

	error = stat(path, stat_buf? &st_bsd: NULL);
	if (error != 0 || stat_buf == NULL)
		return error;

	stcopy(stat_buf, &st_bsd);
	return error;
}

int NSAPI(__lxstat)(int ver, const char * path, struct stat_gnu * stat_buf)
{
	int error;
	struct stat st_bsd;

	error = lstat(path, stat_buf? &st_bsd: NULL);
	if (error != 0 || stat_buf == NULL)
		return error;

	stcopy(stat_buf, &st_bsd);
	return error;
}

int * NSAPI(__errno_location)(void)
{
	return &errno;
}

void NSAPI(sincos)(double x, double *sin0, double *cos0)
{
	if (sin0 != NULL)
		*sin0 = sin(x);

	if (cos0 != NULL)
		*cos0 = cos(x);
}

ssize_t NSAPI(lseek)(int fildes, ssize_t offset, int whence)
{
	return lseek(fildes, offset, whence);
}

int NSAPI(pthread_mutexattr_init)(pthread_mutexattr_t * attr)
{
	*(int *)attr = 0;
	return 0;
}

int NSAPI(pthread_mutexattr_destroy)(pthread_mutexattr_t * attr)
{
	*(int *)attr = -1;
	return 0;
}

int NSAPI(pthread_mutexattr_settype)(pthread_mutexattr_t * attr, int type)
{
	WARN(type == 1, "mutexattr_settype");
	*(int *)attr = PTHREAD_MUTEX_RECURSIVE;
	return 0;
}

static __thread int leak_mutex_exist = 0;
static __thread pthread_mutex_t leak_mutex_obj;

int NSAPI(pthread_mutex_init)(pthread_mutex_t * mutex,
		pthread_mutexattr_t * attr)
{
	int error;
	int * gnu_attr;
	int ** gnu_mutex = (int **)mutex;
	pthread_mutexattr_t * bsd_attr_obj = NULL;

	WARN(init_done == 1, "mutex_init 0");

	if (attr != NULL) {
		gnu_attr = (int *)attr;
		WARN(*gnu_attr ==  PTHREAD_MUTEX_RECURSIVE, "mutex_init 1");
		bsd_attr_obj = &mutexattr_fixobj;
	}

	error = pthread_mutex_init(mutex, bsd_attr_obj);

	WARN(error == 0, "mutex_init 2");
	return error;
}

int NSAPI(pthread_mutex_destroy)(pthread_mutex_t * mutex)
{
	int error;
	int ** gnu_mutex = (int **)mutex;

	error = pthread_mutex_destroy(mutex);
	if (error == 0)
		return error;

	WARN(error == EBUSY, "mutex_destroy 0");
	while (leak_mutex_exist == 1) {
		error = pthread_mutex_destroy(&leak_mutex_obj);
		if (error == 0) {
			leak_mutex_exist = 0;
			break;
		}
		WARN(error == EBUSY, "mutex_destroy 1");
		pthread_mutex_lock(&leak_mutex_obj);
		pthread_mutex_unlock(&leak_mutex_obj);
	}

	leak_mutex_obj = *mutex;
	leak_mutex_exist = 1;
	return error;
}

int NSAPI(pthread_cond_destroy)(pthread_cond_t * cond)
{
	int error;
	error = pthread_cond_destroy(cond);
	WARN(error == 0, "cond_destroy");
	return error;
}

int NSAPI(pthread_getattr_np)(pthread_t pid, pthread_attr_t * dst)
{
	return pthread_attr_get_np(pid, dst);
}

FILE * NSAPI(fopen64)(const char * path, const char * mode)
{
	return fopen(path, mode);
}

int NSAPI(fseeko64)(FILE * stream, size_t offset, int whence)
{
	return fseeko(stream, offset, whence);
}

size_t NSAPI(ftello64)(FILE * stream)
{
	return ftello(stream);
}

static char __guard_locale[256] = {"0123456789"};
void * NSAPI(__newlocale)(int mask, const char *locale, void *base)
{
	dbg_trace("%x %s base = %p\n", mask, locale, base);
	return __guard_locale;
}

int NSAPI(__uselocale)(void *base)
{
	dbg_trace("__uselocale is calling: %p %p\n", base, __guard_locale);
	return (int)base;
}

int NSAPI(__freelocale)(void *base)
{
	dbg_trace("__freelocale is calling: %p %p\n", base, __guard_locale);
	return 0;
}

unsigned short NSAPI(__wctype_l)(const char *property, void *locale)
{
	dbg_trace("__wctype_l: %s %p\n", property, locale);
	return 0;
}

const void *
fixup_lookup(const char * name, int in_plt)
{
#define MAP(old, new)  if (strcmp(name, #old) == 0) return (const void *)new;
#define FIXUP(f) MAP(f, f##_fixup)
	MAP(stdin, stdin);
	MAP(stderr, stderr);
	MAP(stdout, stdout);
	MAP(fopen64, fopen);
	FIXUP(mmap);
	FIXUP(dlopen);
	FIXUP(__newlocale);
	FIXUP(__uselocale);
	FIXUP(__freelocale);
	FIXUP(__wctype_l);
	FIXUP(setlocale);
	FIXUP(getaddrinfo);
	FIXUP(freeaddrinfo);
	FIXUP(connect);
	FIXUP(dlsym);
	FIXUP(dlclose);
	FIXUP(sysconf);
	FIXUP(lseek);
	FIXUP(semop);
	FIXUP(semget);
	FIXUP(semctl);
	FIXUP(uname);
	FIXUP(readdir);
	FIXUP(mprotect);
	FIXUP(sysinfo);
	FIXUP(shmctl);
	FIXUP(shmget);
	FIXUP(shmat);
	FIXUP(sincos);
	FIXUP(__lxstat64);
	FIXUP(__lxstat);
	FIXUP(__errno_location);
	FIXUP(__xstat64);
	FIXUP(__xstat);
	FIXUP(__newlocale);
	FIXUP(__uselocale);
	FIXUP(__freelocale);
	FIXUP(__wctype_l);

	FIXUP(pthread_mutex_init);
	FIXUP(pthread_mutex_destroy);
	FIXUP(pthread_cond_destroy);

	FIXUP(pthread_mutexattr_init);
	FIXUP(pthread_mutexattr_settype);
	FIXUP(pthread_mutexattr_destroy);

	MAP(ftello64, ftello);
	MAP(fseeko64, fseeko);
	MAP(pthread_getattr_np, pthread_attr_get_np);
#undef FIXUP
#undef MAP

	return NULL;
}

