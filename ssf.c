#ifdef __OpenBSD__
#define _BSD_SOURCE
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifdef __OpenBSD__
#include <getopt.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <uvm/uvm_extern.h>
#endif

#include "ascii"

/* sizes */
#define MAXLINE 256
#define BUFSIZE 512

#define CLR_RESET "\033[0m"
#define CLR_BLUE "\033[1;34m"
#define CLR_CYAN "\033[1;36m"
#define CLR_GREEN "\033[1;32m"
#define CLR_MAGENTA "\033[1;35m"
#define CLR_RED "\033[1;31m"
#define CLR_WHITE "\033[1;37m"
#define CLR_YELLOW "\033[1;33m"

#define LEN(x) (sizeof(x) / sizeof(*(x)))
#define MAXIMUM(a,b) ((a) > (b) ? (a) : (b))
#define UNKNOWN(buf) sset((buf), sizeof(buf), "unknown")
#define APPEND(buf, fmt, ...) \
	do { \
		size_t __off = strlen(buf); \
		if (__off < sizeof(buf)) \
			snprintf((buf) + __off, sizeof(buf) - __off, fmt, __VA_ARGS__); \
	} while (0)

#ifdef __OpenBSD__
static int sysctl_hw (const char *, void *, size_t *);
#endif

struct Info
{
  char hostname[MAXLINE], kernel[MAXLINE], uptime[MAXLINE];
  char cpu[MAXLINE], memory[MAXLINE], load[MAXLINE];
  char distro[MAXLINE], shell[MAXLINE], terminal[MAXLINE], user[MAXLINE];
  char ip[16];
};

struct AsciiEntry
{
  const char *needle;
  const char **art;
  const char *color;
};

static void sset (char *, size_t, const char *);
static void usage (void);
static void hostname (struct Info *);
static void kernel (struct Info *);
static void uptime (struct Info *);
static void cpu (struct Info *);
static void memory (struct Info *);
static void ip (struct Info *);
static void load (struct Info *);
static void distro (struct Info *);
static void shell (struct Info *);
static void terminal (struct Info *);
static void user (struct Info *);
static void collect (struct Info *);
static void display (struct Info *, int, int, char *);
static const char **asciisel (const char *, const char **);
static int asciimatch (const char *, const char *);

static char *argv0;

static const struct AsciiEntry ascii_entries[] = {
  {"alpine", ascii_alpine, CLR_CYAN},
  {"android", ascii_android, CLR_GREEN},
  {"arch", ascii_arch, CLR_CYAN},
  {"arco", ascii_arco, CLR_BLUE},
  {"artix", ascii_artix, CLR_CYAN},
  {"centos", ascii_centos, CLR_MAGENTA},
  {"debian", ascii_debian, CLR_RED},
  {"endeavour", ascii_endeavour, CLR_MAGENTA},
  {"fedora", ascii_fedora, CLR_BLUE},
  {"freebsd", ascii_freebsd, CLR_RED},
  {"gentoo", ascii_gentoo, CLR_MAGENTA},
  {"linux mint", ascii_linux_mint, CLR_GREEN},
  {"macos", ascii_macos, CLR_WHITE},
  {"mac os", ascii_macos, CLR_WHITE},
  {"manjaro", ascii_manjaro, CLR_GREEN},
  {"nixos", ascii_nixos, CLR_BLUE},
  {"openbsd", ascii_openbsd, CLR_YELLOW},
  {"opensuse", ascii_opensuse, CLR_GREEN},
  {"pop!_os", ascii_pop_os, CLR_CYAN},
  {"pop os", ascii_pop_os, CLR_CYAN},
  {"slackware", ascii_slackware, CLR_BLUE},
  {"solus", ascii_solus, CLR_BLUE},
  {"ubuntu", ascii_ubuntu, CLR_MAGENTA},
  {"void", ascii_void, CLR_GREEN},
  {"linux", ascii_linux, CLR_WHITE}
};

static void
sset (char *dst, size_t dstsz, const char *src)
{
  if (!dstsz)
    return;
#ifdef __OpenBSD__
  strlcpy (dst, src, dstsz);
#else
  snprintf (dst, dstsz, "%s", src);
#endif
}

#ifdef __OpenBSD__
static int
sysctl_hw (const char *name, void *out, size_t *lenp)
{
  int mib[2];
  int miblen = 2;

  if (!strcmp (name, "hw.model")) {
    mib[0] = CTL_HW;
    mib[1] = HW_MODEL;
  }
  else if (!strcmp (name, "hw.ncpu")) {
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
  }
  else if (!strcmp (name, "hw.physmem64")) {
    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM64;
  }
  else {
    return -1;
  }
  return sysctl (mib, (u_int) miblen, out, lenp, NULL, 0);
}

#endif

static void
usage (void)
{
  fprintf (stderr, "usage: %s [-m] [-s] [-k key] [-h] [-v]\n", argv0);
  exit (1);
}

static void
ip (struct Info *i)
{
  struct ifaddrs *ifap, *ifa;

  if (getifaddrs (&ifap) < 0) {
    strcpy (i->ip, "0.0.0.0");
    return;
  }
  for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
      continue;

    if (strcmp (ifa->ifa_name, "lo0") == 0
	|| strcmp (ifa->ifa_name, "lo") == 0)
      continue;
    struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
    inet_ntop (AF_INET, &sa->sin_addr, i->ip, sizeof (i->ip));
    break;
  }
  freeifaddrs (ifap);
  if (!i->ip[0])
    strcpy (i->ip, "0.0.0.0");

}

static void
hostname (struct Info *i)
{
  if (gethostname (i->hostname, sizeof (i->hostname)) < 0)
    UNKNOWN (i->hostname);
}

static void
kernel (struct Info *i)
{
  struct utsname u;
  if (uname (&u) < 0 || !*u.sysname)
    UNKNOWN (i->kernel);
  else
    snprintf (i->kernel, sizeof (i->kernel), "%s %s", u.sysname, u.release);
}

static void
uptime (struct Info *i)
{
#if defined(__linux__)
  struct sysinfo s;
  int d, h, m;
  if (sysinfo (&s) < 0) {
    UNKNOWN (i->uptime);
    return;
  }
  d = s.uptime / 86400;
  h = (s.uptime % 86400) / 3600;
  m = (s.uptime % 3600) / 60;
#elif defined(__OpenBSD__)
  struct timeval boottime;
  time_t now, up;
  int mib[2] = { CTL_KERN, KERN_BOOTTIME };
  size_t size = sizeof (boottime);
  int d, h, m;

  if (sysctl (mib, 2, &boottime, &size, NULL, 0) < 0 || !boottime.tv_sec) {
    UNKNOWN (i->uptime);
    return;
  }
  if ((now = time (NULL)) == (time_t) - 1) {
    UNKNOWN (i->uptime);
    return;
  }
  up = now - boottime.tv_sec;
  if (up < 0)
    up = 0;
  d = up / 86400;
  h = (up % 86400) / 3600;
  m = (up % 3600) / 60;
#else
  UNKNOWN (i->uptime);
  return;
#endif
  if (d)
    snprintf (i->uptime, sizeof (i->uptime), "%dd %dh %dm", d, h, m);
  else if (h)
    snprintf (i->uptime, sizeof (i->uptime), "%dh %dm", h, m);
  else
    snprintf (i->uptime, sizeof (i->uptime), "%dm", m);
}

static void
cpu (struct Info *i)
{
#if defined(__linux__)
  FILE *f;
  char buf[BUFSIZE], model[MAXLINE] = "unknown";
  int cores = 0;

  if (!(f = fopen ("/proc/cpuinfo", "r"))) {
    UNKNOWN (i->cpu);
    return;
  }
  while (fgets (buf, sizeof (buf), f))
    if (!strncmp (buf, "model name", 10)) {
      if ((buf[strcspn (buf, "\n")] = 0, strchr (buf, ':')))
	sset (model, sizeof (model), strchr (buf, ':') + 2);
    }
    else if (!strncmp (buf, "processor", 9))
      cores++;

  fclose (f);
  sset (i->cpu, sizeof (i->cpu), model);
  if (cores)
    APPEND (i->cpu, " (%d cores)", cores);

#elif defined(__OpenBSD__)
  char model[MAXLINE] = "unknown";
  int cores = 0;
  size_t len;

  len = sizeof (model);
  if (sysctl_hw ("hw.model", model, &len) == -1 || !*model) {
    UNKNOWN (i->cpu);
    return;
  }
  len = sizeof (cores);
  if (sysctl_hw ("hw.ncpu", &cores, &len) == -1)
    cores = 0;
  sset (i->cpu, sizeof (i->cpu), model);
  if (cores > 0)
    APPEND (i->cpu, " (%d cores)", cores);
#else
  UNKNOWN (i->cpu);
#endif
}

static void
memory (struct Info *i)
{
#if defined(__linux__)
  FILE *f;
  char buf[BUFSIZE];
  long total = 0, avail = 0;

  if (!(f = fopen ("/proc/meminfo", "r"))) {
    UNKNOWN (i->memory);
    return;
  }
  while (fgets (buf, sizeof (buf), f) && (!total || !avail))
    if (!strncmp (buf, "MemTotal:", 9))
      sscanf (buf, "MemTotal: %ld", &total);
    else if (!strncmp (buf, "MemAvailable:", 13))
      sscanf (buf, "MemAvailable: %ld", &avail);

  fclose (f);
  if (total && avail) {
    long used = (total - avail) / 1024;
    long tmb = total / 1024;
    snprintf (i->memory, sizeof (i->memory), "%ld/%ld MB", used, tmb);
  }
  else {
    UNKNOWN (i->memory);
  }
#elif defined(__OpenBSD__)
  int mib[2];
  struct uvmexp uvmexp;
  unsigned long long total, used;
  size_t len;
  long pagesize;

  mib[0] = CTL_HW;
  mib[1] = HW_PHYSMEM64;
  len = sizeof (total);
  if (sysctl (mib, 2, &total, &len, NULL, 0) == -1) {
    UNKNOWN (i->memory);
    return;
  }
  mib[0] = CTL_VM;
  mib[1] = VM_UVMEXP;
  len = sizeof (uvmexp);
  if (sysctl (mib, 2, &uvmexp, &len, NULL, 0) == -1) {
    UNKNOWN (i->memory);
    return;
  }
  if ((pagesize = sysconf (_SC_PAGESIZE)) <= 0) {
    UNKNOWN (i->memory);
    return;
  }
  used =
    (unsigned long long) (uvmexp.active +
			  uvmexp.wired) * (unsigned long long) pagesize;
  snprintf (i->memory, sizeof (i->memory), "%llu/%llu MiB",
	    used / (1024ULL * 1024ULL), total / (1024ULL * 1024ULL));
#else
  UNKNOWN (i->memory);
#endif
}

static void
load (struct Info *i)
{
#if defined(__linux__)
  struct sysinfo s;
  if (sysinfo (&s) < 0) {
    UNKNOWN (i->load);
    return;
  }
  snprintf (i->load, sizeof (i->load), "%.2f %.2f %.2f",
	    s.loads[0] / 65536.0, s.loads[1] / 65536.0, s.loads[2] / 65536.0);
#elif defined(__OpenBSD__)
  double loadavg[3];

  if (getloadavg (loadavg, 3) == -1) {
    UNKNOWN (i->load);
    return;
  }
  snprintf (i->load, sizeof (i->load), "%.2f %.2f %.2f",
	    loadavg[0], loadavg[1], loadavg[2]);
#else
  UNKNOWN (i->load);
#endif
}

static void
distro (struct Info *i)
{
  FILE *f;
  char buf[BUFSIZE], *p, *q;

  *i->distro = 0;
  if ((f = fopen ("/etc/os-release", "r"))) {
    while (fgets (buf, sizeof (buf), f))
      if (!strncmp (buf, "PRETTY_NAME=", 12) && (p = strchr (buf, '"'))) {
	if ((q = strchr (++p, '"'))) {
	  *q = 0;
	  sset (i->distro, sizeof (i->distro), p);
	  break;
	}
      }
    fclose (f);
  }
  if (!*i->distro && (f = fopen ("/etc/issue", "r"))) {
    if (fgets (buf, sizeof (buf), f)) {
      buf[strcspn (buf, "\n\\\\")] = 0;
      sset (i->distro, sizeof (i->distro), buf);
    }
    fclose (f);
  }
  if (!*i->distro) {
    struct utsname u;
    if (uname (&u) == 0 && *u.sysname)
      sset (i->distro, sizeof (i->distro), u.sysname);
    else
      UNKNOWN (i->distro);
  }
}

static void
shell (struct Info *i)
{
  char *s, *b;
  if (!(s = getenv ("SHELL"))) {
    UNKNOWN (i->shell);
    return;
  }
  sset (i->shell, sizeof (i->shell), (b = strrchr (s, '/')) ? b + 1 : s);
}

static void
terminal (struct Info *i)
{
  char *t = getenv ("TERM");
  if (!t) {
    UNKNOWN (i->terminal);
    return;
  }
  sset (i->terminal, sizeof (i->terminal), t);
}

static void
user (struct Info *i)
{
  struct passwd *p = getpwuid (getuid ());
  if (!p || !p->pw_name) {
    UNKNOWN (i->user);
    return;
  }
  sset (i->user, sizeof (i->user), p->pw_name);
}

static void
collect (struct Info *i)
{
  hostname (i);
  kernel (i);
  uptime (i);
  cpu (i);
  memory (i);
  load (i);
  distro (i);
  shell (i);
  terminal (i);
  user (i);
  ip (i);
}

static void
display (struct Info *i, int m, int s, char *k)
{
  char *sep;
  struct
  {
    char *name, *val;
  } fields[] = {
    {"user", i->user}, {"hostname", i->hostname}, 
	{"ip", i->ip}, {"distro", i->distro},
    {"kernel", i->kernel}, {"uptime", i->uptime}, 
	{"shell", i->shell},
    {"terminal", i->terminal}, {"cpu", i->cpu}, 
	{"memory", i->memory},{"load", i->load},
  }, short_fields[] = {
    {"hostname", i->hostname}, {"kernel", i->kernel}, 
	{"uptime", i->uptime},
    {"memory", i->memory}
  };
  const char **ascii;
  const char *color, *line;
  int n, j, ascii_lines, ascii_width, max_lines, len, pad;
  int total, short_total;

  sep = m ? "|" : ": ";
  total = LEN (fields);
  short_total = LEN (short_fields);
  if (k) {
    for (j = 0; j < total; j++)
      if (!strcmp (k, fields[j].name)) {
	printf ("%s\n", fields[j].val);
	return;
      }
    return;
  }
  ascii = asciisel (i->distro, &color);
  ascii_lines = 0;
  ascii_width = 0;
  if (ascii) {
    for (j = 0; ascii[j]; j++) {
      len = (int) strlen (ascii[j]);
      if (len > ascii_width)
	ascii_width = len;
    }
    ascii_lines = j;
  }
  n = s ? short_total : total;
  max_lines = MAXIMUM (ascii_lines, n);
  for (j = 0; j < max_lines; j++) {
    len = 0;
    pad = ascii_width;
    if (j < ascii_lines) {
      line = ascii[j];
      len = (int) strlen (line);
      pad = ascii_width - len;
      if (pad < 0)
	pad = 0;
      printf ("%s%s%s", color, line, CLR_RESET);
      if (pad)
	printf ("%*s", pad, "");
    }
    else if (ascii_width) {
      printf ("%*s", ascii_width, "");
    }
    printf ("  ");
    if (j < n)
      printf ("%s%s%s", (s ? short_fields : fields)[j].name, sep,
	      (s ? short_fields : fields)[j].val);
    putchar ('\n');
  }
}

static const char **
asciisel (const char *distro, const char **color)
{
  int idx, count;

  count = (int) LEN (ascii_entries);
  for (idx = 0; idx < count; idx++)
    if (asciimatch (distro, ascii_entries[idx].needle)) {
      *color = ascii_entries[idx].color;
      return ascii_entries[idx].art;
    }
  *color = CLR_WHITE;
  return ascii_linux;
}

static int
asciimatch (const char *distro, const char *needle)
{
  size_t len;
  const char *p;

  if (!distro || !needle)
    return 0;
  len = strlen (needle);
  if (!len)
    return 0;
  for (p = distro; *p; p++)
    if (!strncasecmp (p, needle, len))
      return 1;
  return 0;
}

int
main (int argc, char **argv)
{
  struct Info info = { 0 };
  int m = 0, s = 0, c;
  char *k = NULL;

  for (argv0 = *argv; (c = getopt (argc, argv, "msk:hv")) != -1;)
    switch (c) {
    case 'm':
      m = 1;
      break;
    case 's':
      s = 1;
      break;
    case 'k':
      k = optarg;
      break;
    case 'h':
      usage ();
      break;
    case 'v':
      puts ("ssf 1.0");
      exit (0);
    default:
      usage ();
    }

  collect (&info);
  display (&info, m, s, k);
  return 0;
}
