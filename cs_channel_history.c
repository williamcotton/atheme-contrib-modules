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
		
		char value[565];
        char list[100];
        
		char message[565];
		sprintf(message, "%s", data->msg);

        //char *message = data->msg;

        char *nick = data->u->nick;
        char *channel = data->c->name;
        
        sprintf(list, "channel_history:%s", channel);
		
        printf(" --- %s ----- <%s> %s\n", channel, nick, message);

		//////
		
		json_object *new_obj;
		json_object *avatar;
		json_object *date;

		// char message[] = ":user16690!~user16690@174.129.220.161 PRIVMSG user75436 :JSON {\"test\":\"dsf sdfsdf sdf sdf\", \"avatar\":\"home/05f3e9d8-5423-4778-b0d9-a9cc07d655c4.jpg\"}";

		// puts(message);

		char jsonIn[565];

		char * pch;
		pch = strtok(message," ");
		int i = 0;
		int isJSONCTCP = 0;
		while (pch != NULL) {
			if (i == 0) {
				if (strcmp(pch, "JSON") == 0) {
					isJSONCTCP = 1;
					puts("\nis a JSON CTCP message");
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
			puts(jsonIn);
			// new_obj = json_tokener_parse(jsonIn);
			// 
			// avatar = json_object_object_get(new_obj, "avatar");
			// const char *avatar_url = json_object_get_string(avatar);
			// printf("\nAvatar URL = %s", avatar_url);
			// 
			// /* Obtain current time as seconds elapsed since the Epoch. */
			// 	    time_t clock = time(NULL);
			// 
			// 	    /* Convert to local time format and print to stdout. */
			// 
			// char *currentTime = ctime(&clock);
			// 	    printf("\nCurrent time is %s", currentTime);
			// 
			// long epoch_time = (long) clock;
			// printf("\nseconds since the Epoch: %ld\n", epoch_time);
			// 
			// json_object *output;
			// 
			// //output = json_object_new_object();
			// json_object_object_add(new_obj, "epoch_time", json_object_new_int(epoch_time));
			// json_object_object_add(new_obj, "time", json_object_new_string(currentTime));
			// 
			// printf("\n%s", json_object_to_json_string(new_obj));
		}
		
		////////
        
        sprintf(value, "%s:::%s", nick, message);
        printf("%s\n", value);
        
        redisContext *redis = redisConnect("127.0.0.1", 6379);
        redisReply *reply;
        
        reply = redisCommand(redis, "RPUSH %s %s", list, value);
        freeReplyObject(reply);

        reply = redisCommand(redis, "LLEN %s", list);
        if (reply->integer > 50) {
            redisCommand(redis, "LPOP %s", list);
        }
        freeReplyObject(reply);

        redisFree(redis);
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
    char *channel = c->name;
    
    sprintf(list, "channel_history:%s", channel);
    
    printf(" -------- %s joined %s\n", nick, channel);
    
    redisContext *redis = redisConnect("127.0.0.1", 6379);
    redisReply *reply;
    
    reply = redisCommand(redis,"LRANGE %s 0 -1", list);   
    for (int i = 0; i < reply->elements; i++) {
        printf("\n --- %s", reply->element[i]->str);
        msg(chansvs.nick, nick, "%s:::%s", channel, reply->element[i]->str);
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