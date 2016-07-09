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
#include "mrb_procutil.h"

#include <unistd.h>

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

void mrb_mruby_procutil_gem_init(mrb_state *mrb)
{
    struct RClass *procutil;
    procutil = mrb_define_module(mrb, "Procutil");

    mrb_define_module_function(mrb, procutil, "sethostname", mrb_procutil_sethostname, MRB_ARGS_REQ(1));

    DONE;
}

void mrb_mruby_procutil_gem_final(mrb_state *mrb)
{
}
