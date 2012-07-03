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

#include <json/json.h> // json-c library... make sure that everything is in /usr/local/lib and /usr/local/include. There might be explicit moving of files involved.
#include <hiredis/hiredis.h> // redis library... also, make sure the .a files are in /usr/local/lib and the .h files in /usr/local/include.
// if you're having issues loading the module, check that LD_LIBRARY_PATH is set and includes where the above libs are, most likely in /usr/local
// also, make sure -lhiredis and -ljson are in the lib flags in the Makefile...

/*

	The basic idea of this module is this:
	
	- ChanServ listens on all registered channels for CTCP JSON messages and stores the last 12 hours of messages in a Redis list.
	- When a user joins a registered channel, they are sent CTCP JSON messages for the last 12 hours.
	- Messages older than 12 hours are removed from the list.
	- The incoming JSON message is given "nick", "channel", "epoch_time", and "time" attributes.
	- The module remains agnostic of the remaining JSON attributes, allowing for any message type to be created for the client side of things.

*/

DECLARE_MODULE_V1
(
	"contrib/cs_channel_history", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Cotton <williamcotton@gmail.com>"
);

void elog(const char *message)
{
    if (false) {
        puts(message);
    }
}

static void
on_channel_message(hook_cmessage_data_t *data)
{
    
    if (data != NULL && data->msg != NULL) {	

        elog("nick and channel strings");
        // nick and channel strings
        char *nick = data->u->nick;
        char *channel = data->c->name;

        elog("the incoming message");
        // the incoming message
        char check_message[565];
        sprintf(check_message, "%s", data->msg);

        /*
        	This routine checks to see if the incoming message is a CTCP of type JSON, and if it is, stores the JSON object in jsonIn.

        	in ruby:

        	messages = check_message.split(" ")
        	if messages[0] == "JSON"
        		jsonIn = messages[1..-1].join(" ")
        	end

        */
        elog("parse for CTCP JSON");
        char jsonIn[565];
        char *pch;
        pch = strtok(check_message," ");
        int i = 0;
        int isJSONCTCP = 0;
        while (pch != NULL) {
            if (i == 0) { // if messages[0] == "JSON"
                if (strcmp(pch, "JSON") == 0) { // "JSON" has a \001 as that space, be warry of that!!!
                    isJSONCTCP = 1;
                }
                else { // might as well get out of here, right?
                    elog("not a valid CTCP JSON message, leaving");
                    return;
                }
            }
            if (i == 1 && isJSONCTCP == 1) { // messages[1..-1]
                strcpy(jsonIn, pch); 
            }
            if (i > 1 && isJSONCTCP == 1) { // .join(" ")
            	strcat(jsonIn, " ");
            	strcat(jsonIn, pch);
            }
            pch = strtok(NULL, " ");
            i++;
        }

        //printf("jsonIn: %s\n", jsonIn);
        //printf("strlen(jsonIn): %i\n", (int)strlen(jsonIn));

        elog("check if it was a CTCP JSON message");
        if ((int)strlen(jsonIn) > 0) { // if it was a JSON CTCP

            elog("parse the JSON");
            // parse the JSON
            json_object *new_obj;
            new_obj = json_tokener_parse(jsonIn);

            elog("check to see if it is valid JSON");
            // check to see if it is valid JSON
            json_type o_type;
            o_type = json_object_get_type(new_obj);
            if (o_type == json_type_null) {
                elog("not valid json, leaving");
                return;
            }

            elog("getting timestamps");
            // getting timestamps
            time_t clock = time(NULL);
            char *currentTime = ctime(&clock);
            long epoch_time = (long)clock;

            elog("adding to the incoming JSON a bunch of fields that shouldn't be set by clients");
            // adding to the incoming JSON a bunch of fields that shouldn't be set by clients
            json_object_object_add(new_obj, "epoch_time", json_object_new_int(epoch_time));
            json_object_object_add(new_obj, "time", json_object_new_string(currentTime));
            json_object_object_add(new_obj, "nick", json_object_new_string(nick));
            json_object_object_add(new_obj, "channel", json_object_new_string(channel));


            elog("I don't totally know why I need to load up the string like this... I need to know more about C!");
            // I don't totally know why I need to load up the string like this... I need to know more about C!
            char jsonSave[565];
            sprintf(jsonSave, "%s", json_object_to_json_string(new_obj));

            elog("connect to Redis");
            // connect to Redis
            redisContext *redis = redisConnect("127.0.0.1", 6379);
            redisReply *reply;

            elog("our Redis list name, based on the channel");
            // our Redis list name, based on the channel
            char list[100];
            sprintf(list, "channel_history:%s", channel);

            elog("save the JSON to the list");
            // save the JSON to the list
            reply = redisCommand(redis, "RPUSH %s %s", list, jsonSave);
            freeReplyObject(reply);

            elog("free at last!");
            // free at last!
            redisFree(redis);
        }

    }
}

static void
on_channel_join(hook_channel_joinpart_t *hdata)
{

    elog("get our user and channel structs");
    // get our user and channel structs
    channel_t *c = hdata->cu->chan;
    user_t *u = hdata->cu->user;
    char *nick = u->nick;
    char *channel = c->name;

    elog("get timestamps");
    // get timestamps
    time_t clock = time(NULL);
    int current_epoch_time = (int)(long)clock;

    elog("declare out json_objects");
    // declare our json_objects
    json_object *new_obj;
    json_object *epoch_time_obj;

    elog("connect to Redis");
    // connect to Redis
    redisContext *redis = redisConnect("127.0.0.1", 6379);
    redisReply *reply;

    elog("format our Redis list key");
    // format our Redis list key
    char list[100];
    sprintf(list, "channel_history:%s", channel);

    elog("get the list of messages");
    // get the list of messages
    reply = redisCommand(redis,"LRANGE %s 0 -1", list); 

    elog("send out stream start");

    msg(chansvs.nick, nick, "JSON {\"channel\":\"%s\", \"start_channel_history\":\"true\"} ", channel); 

    elog("loop through the list");
    // loop through the list
    for (int i = 0; i < reply->elements; i++) {

        elog("parse the JSON");
        // parse the JSON
        new_obj = json_tokener_parse(reply->element[i]->str);

        elog("make sure it is valid JSON, if not, remove it from the Redis list");
        // make sure it is valid JSON, if not, remove it from the Redis list
        json_type o_type;
        o_type = json_object_get_type(new_obj);
        if (o_type == json_type_null) {
            redisCommand(redis, "LREM %s %d %s", list, 0, reply->element[i]->str);
            continue;
        }

        elog("sanity check 1");
        // sanity check
        epoch_time_obj = json_object_object_get(new_obj, "epoch_time");	
        if ((long)epoch_time_obj == 1) { // I don't know why this does this... it being set to '1' was the trick... THIS IS GHOST CODE, PLEASE INSPECT AND FIGURE IT OUT, POSSIBLE REMOVE IT!!!
            redisCommand(redis, "LREM %s %d %s", list, 0, reply->element[i]->str);
            continue;
        }

        elog("sanity check 2");
        // sanity check
        if (is_error(epoch_time_obj)) {
            continue;
        }

        elog("sanity check 3");
        // sanity check
        json_type type;
        type = json_object_get_type(epoch_time_obj);
        if (json_object_is_type(epoch_time_obj, json_type_int)) {

            elog("get the time that the message was created");
            // get the time that the message was created
            int msg_epoch_time = json_object_get_int(epoch_time_obj);

            elog("remove messages that are older than 12 hours");
            // remove messages that are older than 12 hours
            int max_hours = 12;
            int max_seconds = 60*60*max_hours;
            int difference;
            difference = current_epoch_time - msg_epoch_time;
            if (difference > max_seconds) {
                //printf("current_epoch_time: %d\n", current_epoch_time);
                //printf("msg_epoch_time: %d\n", msg_epoch_time);
                //printf("removing message with timestamp: %s\n", reply->element[i]->str);
                redisCommand(redis, "LREM %s %d %s", list, 0, reply->element[i]->str);
                continue;
            }


            elog("message the user who joined a CTCP JSON message");
            // message the user who joined a CTCP JSON message
            msg(chansvs.nick, nick, "JSON %s", reply->element[i]->str); // "JSON" has a \001 as that space, be warry of that!!!

        }

    }

    msg(chansvs.nick, nick, "JSON {\"channel\":\"%s\", \"end_channel_history\":\"true\"} ", channel); 

    elog("free at last!");
    // free at last!
    freeReplyObject(reply);   
    redisFree(redis);
}

void _modinit(module_t *m)
{
    // register a handler function for when a user posts a message in a channel
    hook_add_event("channel_message");
    hook_add_channel_message(on_channel_message);

    // register a handler function for when a user joins a channel
    hook_add_event("channel_join");
    hook_add_channel_join(on_channel_join);
}

void _moddeinit(module_unload_intent_t intent)
{
    // unregister the handlers... important for modreload
    hook_del_channel_message(on_channel_message);
    hook_del_channel_join(on_channel_join);
}