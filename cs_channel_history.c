/*
 * Copyright (c) 2012 William Cotton
 *
 */

#include "atheme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stddef.h>
#include <time.h>

#include <json/json.h>
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
		
        char list[100];
        
		char check_message[565];
		sprintf(check_message, "%s", data->msg);

        char *nick = data->u->nick;
        char *channel = data->c->name;
        
        sprintf(list, "channel_history:%s", channel);

		char jsonIn[565];

		char * pch;
		pch = strtok(check_message," ");
		int i = 0;
		int isJSONCTCP = 0;
		while (pch != NULL) {
			if (i == 0) {
				if (strcmp(pch, "JSON") == 0) { // "JSON" has a \001 as that space, be warry of that!!!
					isJSONCTCP = 1;
				}
			}
			if (i == 1 && isJSONCTCP == 1) {
				strcpy(jsonIn, pch);
			}
			if (i > 1 && isJSONCTCP == 1) {
				strcat(jsonIn, " ");
				strcat(jsonIn, pch);
			}
			pch = strtok (NULL, " ");
			i++;
		}

		if (strlen(jsonIn) > 0) {
			
			json_object *new_obj;
			
			new_obj = json_tokener_parse(jsonIn);

			time_t clock = time(NULL);
			
			char *currentTime = ctime(&clock);
			long epoch_time = (long) clock;

			json_object_object_add(new_obj, "epoch_time", json_object_new_int(epoch_time));
			json_object_object_add(new_obj, "time", json_object_new_string(currentTime));
			json_object_object_add(new_obj, "nick", json_object_new_string(nick));
			json_object_object_add(new_obj, "channel", json_object_new_string(channel));
			
			char jsonSave[565];
			
			sprintf(jsonSave, "%s", json_object_to_json_string(new_obj));
			
	        printf("\n%s", jsonSave);

	        redisContext *redis = redisConnect("127.0.0.1", 6379);
	        redisReply *reply;

	        reply = redisCommand(redis, "RPUSH %s %s", list, jsonSave);
	        freeReplyObject(reply);

	        reply = redisCommand(redis, "LLEN %s", list);
	        if (reply->integer > 5) {
	            redisCommand(redis, "LPOP %s", list);
	        }
	        freeReplyObject(reply);

	        redisFree(redis);
		}
        
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
	
	time_t clock = time(NULL);
	int current_epoch_time = (int)(long)clock;
	
	printf("\ncurrent_epoch_time: %d", current_epoch_time);
	
    char list[100];
    char *message;
	
    char *nick = cu->user->nick;
    char *channel = c->name;
    
    sprintf(list, "channel_history:%s", channel);
    
    printf(" -------- %s joined %s\n", nick, channel);
    
    redisContext *redis = redisConnect("127.0.0.1", 6379);
    redisReply *reply;

	json_object *new_obj;
    
    reply = redisCommand(redis,"LRANGE %s 0 -1", list);   
    for (int i = 0; i < reply->elements; i++) {
	
		new_obj = json_tokener_parse(reply->element[i]->str);
		
		json_object *epoch_time_obj;
		epoch_time_obj = json_object_object_get(new_obj, "epoch_time");
		
		if (epoch_time_obj) {
			puts("2.5");
			int msg_epoch_time = json_object_get_int(epoch_time_obj);
			printf("\nepoch_time: %d", msg_epoch_time);

			int difference;
			difference = current_epoch_time - msg_epoch_time;
			printf("\ndifference: %d", difference);
			
	        printf("\n%s", reply->element[i]->str);
	        msg(chansvs.nick, nick, "JSON %s", reply->element[i]->str); // "JSON" has a \001 as that space, be warry of that!!!
		}

    }
    
    
    
    freeReplyObject(reply);
    
    redisFree(redis);
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