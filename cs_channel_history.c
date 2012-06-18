/*
 * Copyright (c) 2012 William Cotton
 *
 */

#include "atheme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <hiredis/hiredis.h>

DECLARE_MODULE_V1
(
	"contrib/cs_channel_history", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Cotton <williamcotton@gmail.com>"
);

static void
on_channel_message(hook_cmessage_data_t *data)
{
	if (data != NULL && data->msg != NULL)
	{
		mychan_t *mc = MYCHAN_FROM(data->c);
		metadata_t *md;
		
		char value[565];
        char list[100];
        
        

        char *message = data->msg;

        char *nick = data->u->nick;
        char *room = data->c->name;
        
        sprintf(list, "channel_history:%s", room);
		
        printf(" --- %s ----- <%s> %s\n", room, nick, message);
        
        sprintf(value, "%s:::%s", nick, message);
        printf("%s\n", value);
        
        redisContext *redis = redisConnect("127.0.0.1", 6379);
        redisReply *reply;
        
        // reply = redisCommand(redis, "RPUSH %s %s", list, value);
        // freeReplyObject(reply);
        // 
        // reply = redisCommand(redis, "LLEN %s", list);
        // if (reply->integer > 5) {
        //     redisCommand(redis, "LPOP %s", list);
        // }
        // freeReplyObject(reply);
        // 
        // redisFree(redis);
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
	
    char list[100];
    char *message;
	
    char *nick = cu->user->nick;
    char *room = c->name;
    
    sprintf(list, "channel_history:%s", room);
    
    printf(" -------- %s joined %s\n", nick, room);
    
    redisContext *redis = redisConnect("127.0.0.1", 6379);
    redisReply *reply;
    
    // reply = redisCommand(redis,"LRANGE %s 0 -1", list);   
    // for (int i = 0; i < reply->elements; i++) {
    //     printf("\n --- %s", reply->element[i]->str);
    // }
    // freeReplyObject(reply);
    // 
    // redisFree(redis);
}

void _modinit(module_t *m)
{
	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);
	
    hook_add_event("channel_join");
    hook_add_channel_join(on_channel_join);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_channel_message(on_channel_message);
	hook_del_channel_join(on_channel_join);
}