/*
** mrb_procutil.c - Procutil class
**
** Copyright (c) Uchio Kondo 2016
**
** See Copyright Notice in LICENSE
*/

// clang-format off
#include "mruby.h"
#include "mruby/data.h"
#include "mruby/error.h"
#include "mruby/string.h"
#include "mruby/array.h"
#include "mrb_procutil.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
// clang-format on

#define DONE mrb_gc_arena_restore(mrb, 0);

#define SYS_FAIL_MESSAGE_LENGTH 2048

static void mrb_procutil_sys_fail(mrb_state *mrb, int error_no, const char *fmt, ...)
{
  char buf[1024];
  char arg_msg[SYS_FAIL_MESSAGE_LENGTH];
  char err_msg[SYS_FAIL_MESSAGE_LENGTH];
  char *ret;
  va_list args;

  va_start(args, fmt);
  vsnprintf(arg_msg, SYS_FAIL_MESSAGE_LENGTH, fmt, args);
  va_end(args);

  if ((ret = strerror_r(error_no, buf, 1024)) == NULL) {
    snprintf(err_msg, SYS_FAIL_MESSAGE_LENGTH, "[BUG] strerror_r failed. errno: %d message: %s", errno, arg_msg);
    mrb_sys_fail(mrb, err_msg);
  }

  snprintf(err_msg, SYS_FAIL_MESSAGE_LENGTH, "sys failed. errno: %d message: %s mrbgem message: %s", error_no, ret,
           arg_msg);
  mrb_sys_fail(mrb, err_msg);
}

static mrb_value mrb_procutil_sethostname(mrb_state *mrb, mrb_value self)
{
  char *newhostname;
  mrb_int len;
  mrb_get_args(mrb, "s", &newhostname, &len);

  if (sethostname(newhostname, len) < 0) {
    mrb_procutil_sys_fail(mrb, errno, "sethostname failed.");
  }
  return mrb_str_new(mrb, newhostname, len);
}

static mrb_value mrb_procutil_setsid(mrb_state *mrb, mrb_value self)
{
  int ret = setsid();

  if (ret == -1) {
    mrb_procutil_sys_fail(mrb, errno, "setsid failed.");
  }

  return mrb_fixnum_value(ret);
}

#define TRY_REOPEN(fp, newfile, mode, oldfp)                                                                           \
  fp = freopen(newfile, mode, oldfp);                                                                                  \
  if (fp == NULL)                                                                                                      \
  mrb_procutil_sys_fail(mrb, errno, "freopen failed")

/* Force to exit forked mruby process when dup2 failed */
#define TRY_DUP2(oldfd, newfd)                                                                                         \
  if (dup2(oldfd, newfd) < 0)                                                                                          \
  mrb_procutil_sys_fail(mrb, errno, "dup2 failed")

static mrb_value mrb_procutil_daemon_fd_reopen(mrb_state *mrb, mrb_value self)
{
  FILE *fp;

  TRY_REOPEN(fp, "/dev/null", "r", stdin);
  TRY_REOPEN(fp, "/dev/null", "w", stdout);
  TRY_REOPEN(fp, "/dev/null", "w", stderr);

  return mrb_true_value();
}

static mrb_value mrb_procutil_fd_reopen3(mrb_state *mrb, mrb_value self)
{
  mrb_int stdin_fd = 0, stdout_fd = 1, stderr_fd = 2;

  mrb_get_args(mrb, "|iii", &stdin_fd, &stdout_fd, &stderr_fd);

  TRY_DUP2(stdin_fd, STDIN_FILENO);
  TRY_DUP2(stdout_fd, STDOUT_FILENO);
  TRY_DUP2(stderr_fd, STDERR_FILENO);

  return mrb_true_value();
}

static mrb_value mrb_procutil_mark_cloexec(mrb_state *mrb, mrb_value self)
{
  DIR *d = opendir("/proc/self/fd");

  if (!d) {
    mrb_procutil_sys_fail(mrb, errno, "cannot open /proc/self/fd");
  }

  struct dirent *dp;
  while ((dp = readdir(d)) != NULL) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "0") || !strcmp(dp->d_name, "1") ||
        !strcmp(dp->d_name, "2")) {
      // skip ./../stdin/stdout/stderr
    } else {
      char *fileno_str = dp->d_name;
      int fileno = (int)strtol(fileno_str, NULL, 0);
      int flags = fcntl(fileno, F_GETFD);
      if (flags < 0) {
        mrb_procutil_sys_fail(mrb, errno, "fcntl failed (get fd flags)");
      }
      if (fcntl(fileno, F_SETFD, flags | FD_CLOEXEC) < 0) {
        mrb_procutil_sys_fail(mrb, errno, "fcntl failed (set fd FD_CLOEXEC)");
      }
    }
  }
  closedir(d);
  return mrb_nil_value();
}

static mrb_value mrb_procutil___system4(mrb_state *mrb, mrb_value self)
{
  int stdin_fd, stdout_fd, stderr_fd;
  int exit_status = -1, check_status;
  char *cmd;
  pid_t pid;
  mrb_value ret;

  mrb_get_args(mrb, "ziii", &cmd, &stdin_fd, &stdout_fd, &stderr_fd);

  pid = fork();
  if (pid == -1) {
    mrb_procutil_sys_fail(mrb, errno, "fork failed.");
  } else if (pid == 0) {
    TRY_DUP2(stdin_fd, STDIN_FILENO);
    TRY_DUP2(stdout_fd, STDOUT_FILENO);
    TRY_DUP2(stderr_fd, STDERR_FILENO);

    /* see `man system(3)` */
    execl("/bin/sh", "sh", "-c", cmd, (char *)0);

    mrb_procutil_sys_fail(mrb, errno, "execl failed.");

  } else {
    if (waitpid(pid, &check_status, 0) < 0) {
      mrb_procutil_sys_fail(mrb, errno, "waitpid failed.");
    }

    if (WIFEXITED(check_status)) {
      exit_status = WEXITSTATUS(check_status);
    } else {
      exit_status = -1;
    }
  }

  ret = mrb_ary_new_capa(mrb, 2);
  mrb_ary_push(mrb, ret, mrb_fixnum_value(pid));
  mrb_ary_push(mrb, ret, mrb_fixnum_value(exit_status));
  return ret;
}

void mrb_mruby_procutil_gem_init(mrb_state *mrb)
{
  struct RClass *procutil;
  procutil = mrb_define_module(mrb, "Procutil");

  mrb_define_module_function(mrb, procutil, "sethostname", mrb_procutil_sethostname, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, procutil, "setsid", mrb_procutil_setsid, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, procutil, "daemon_fd_reopen", mrb_procutil_daemon_fd_reopen, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, procutil, "fd_reopen3", mrb_procutil_fd_reopen3, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, procutil, "mark_cloexec", mrb_procutil_mark_cloexec, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, procutil, "__system4", mrb_procutil___system4, MRB_ARGS_REQ(4));

  DONE;
}

void mrb_mruby_procutil_gem_final(mrb_state *mrb)
{
}
