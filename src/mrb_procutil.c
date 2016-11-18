/*
** mrb_procutil.c - Procutil class
**
** Copyright (c) Uchio Kondo 2016
**
** See Copyright Notice in LICENSE
*/

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

#define DONE mrb_gc_arena_restore(mrb, 0);

static mrb_value mrb_procutil_sethostname(mrb_state *mrb, mrb_value self)
{
  char *newhostname;
  int len;
  mrb_get_args(mrb, "s", &newhostname, &len);

  if (sethostname(newhostname, len) < 0){
    mrb_sys_fail(mrb, "sethostname failed.");
  }
  return mrb_str_new(mrb, newhostname, len);
}

static mrb_value mrb_procutil_setsid(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(setsid());
}

#define TRY_REOPEN(fp, newfile, mode, oldfp) \
  fp = freopen(newfile, mode, oldfp);                  \
  if(fp == NULL) mrb_sys_fail(mrb, "freopen failed")

static mrb_value mrb_procutil_daemon_fd_reopen(mrb_state *mrb, mrb_value self)
{
  setsid();
  signal(SIGHUP, SIG_IGN);
  signal(SIGINT, SIG_IGN);

  /* TODO reopen to log file */
  FILE *fp;
  TRY_REOPEN(fp, "/dev/null", "r", stdin);
  TRY_REOPEN(fp, "/dev/null", "w", stdout);
  TRY_REOPEN(fp, "/dev/null", "w", stderr);

  return mrb_true_value();
}

static mrb_value mrb_procutil_mark_cloexec(mrb_state *mrb, mrb_value self)
{
  DIR* d = opendir("/proc/self/fd");
  struct dirent *dp;
  while((dp = readdir(d)) != NULL) {
    if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") ||
       !strcmp(dp->d_name, "0") || !strcmp(dp->d_name, "1") || !strcmp(dp->d_name, "2")) {
      // skip ./../stdin/stdout/stderr
    } else {
      char *fileno_str = dp->d_name;
      int fileno = (int)strtol(fileno_str, NULL, 0);
      int flags = fcntl(fileno, F_GETFD);
      if(flags < 0) {
        mrb_sys_fail(mrb, "fcntl failed (get fd flags)");
      }
      if(fcntl(fileno, F_SETFD, flags | FD_CLOEXEC) < 0) {
        mrb_sys_fail(mrb, "fcntl failed (set fd FD_CLOEXEC)");
      }
    }
  }
  closedir(d);
  return mrb_nil_value();
}

/* Force to exit forked mruby process when dup2 failed */
#define TRY_DUP2(oldfd, newfd)                  \
  if(dup2(oldfd, newfd) < 0) {                  \
    perror("dup2");                             \
    _exit(-1);                                  \
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
    mrb_sys_fail(mrb, "fork failed.");
  } else if (pid == 0) {
    TRY_DUP2(stdin_fd,  STDIN_FILENO);
    TRY_DUP2(stdout_fd, STDOUT_FILENO);
    TRY_DUP2(stderr_fd, STDERR_FILENO);

    /* see `man system(3)` */
    execl("/bin/sh", "sh", "-c", cmd, (char *)0);
  } else {
    if(waitpid(pid, &check_status, 0) < 0) {
      mrb_sys_fail(mrb, "waitpid failed.");
    }

    if(WIFEXITED(check_status)) {
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
    mrb_define_module_function(mrb, procutil, "mark_cloexec", mrb_procutil_mark_cloexec, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, procutil, "__system4", mrb_procutil___system4, MRB_ARGS_REQ(4));

    DONE;
}

void mrb_mruby_procutil_gem_final(mrb_state *mrb)
{
}
