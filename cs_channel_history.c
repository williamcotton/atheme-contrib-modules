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
	//mychan_t *mc = MYCHAN_FROM(c);
	
	chanuser_t *cu = hdata->cu;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	metadata_t *md;
    
    if (cu == NULL)
		return;
	if (!(cu->user->server->flags & SF_EOB))
		return;
	mu = cu->user->myuser;
	mc = MYCHAN_FROM(cu->chan);
	if (mu == NULL || mc == NULL)
		return;
	ca = chanacs_find_literal(mc, entity(mu), 0);
	if (ca == NULL || ca->level & CA_AKICK)
		return;
    printf(" -- xx -- ");
    printf(" -------- %s joined %s\n", cu->user->nick, c->name);
    
    REDIS redis;
    REDIS_INFO info;
    
    redis = credis_connect("localhost", 6379, 10000);
    
    int i;
    long t;
    int num = 100;
    printf("Sending %d 'set' commands ...\n", num);
    timer(1);
    for (i=0; i<num; i++) {
    if (credis_set(redis, "kalle", "qwerty") != 0)
      printf("get returned error\n");
    }
    t = timer(0);
    printf("done! Took %.3f seconds, that is %ld commands/second\n", ((float)t)/1000, (num*1000)/t);
    
    /* close connection to redis server */
    credis_close(redis);
}

void _modinit(module_t *m)
{
	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);
	
    // hook_add_event("channel_join");
    // hook_add_channel_join(on_channel_join);
	
	hook_add_event("channel_join");
	hook_add_first_channel_join(on_channel_join);
	
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
