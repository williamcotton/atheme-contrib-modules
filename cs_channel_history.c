/*
 * Copyright (c) 2012 William Cotton
 *
 */

#include "atheme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <credis.h>

DECLARE_MODULE_V1
(
	"contrib/cs_channel_history", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Cotton <williamcotton@gmail.com>"
);

long timer(int reset) 
{
  static long start=0; 
  struct timeval tv;

  gettimeofday(&tv, NULL);

  /* return timediff */
  if (!reset) {
    long stop = ((long)tv.tv_sec)*1000 + tv.tv_usec/1000;
    return (stop - start);
  }

  /* reset timer */
  start = ((long)tv.tv_sec)*1000 + tv.tv_usec/1000;

  return 0;
}

static void
on_channel_message(hook_cmessage_data_t *data)
{
	if (data != NULL && data->msg != NULL)
	{
		mychan_t *mc = MYCHAN_FROM(data->c);
		metadata_t *md;
		
        printf(" -------- <%s> %s\n", data->u->nick, data->msg);

	}
}

static void
on_channel_join(hook_channel_joinpart_t *hdata)
{
    channel_t *c = hdata->cu->chan;
    user_t *u = hdata->cu->user;
	
	chanuser_t *cu = hdata->cu;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	metadata_t *md;
    
    printf(" -------- %s joined %s\n", cu->user->nick, c->name);
    
    REDIS redis;
    redis = credis_connect("localhost", 6379, 10000);
    credis_set(redis, "kalle", "qwerty");
    credis_close(redis);
}

void _modinit(module_t *m)
{
	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);
	
    hook_add_event("channel_join");
    hook_add_channel_join(on_channel_join);
	
    // hook_add_event("channel_join");
    // hook_add_first_channel_join(on_channel_join);
	
	//hook_channel_joinpart_t
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_channel_message(on_channel_message);
	hook_del_channel_join(on_channel_join);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
