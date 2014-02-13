/*
To compile main.c to videoconf
===========================================================================================
gcc main.c surulib.c -lpthread `pkg-config --cflags --libs clutter-gtk-0.10 clutter-gst-0.10 gst-rtsp-server-0.10` -g -o videocon

./videocon daserfost rtsp://192.168.50.1:CLIENT_PORT/daserfost
./videocon daserfost rtsp://192.168.50.1:1981/daserfost
where CLIENT_PORT = 1981

*/

/* GNU Standard C Library */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>	/* POSIX thread library */
#include <sys/types.h>

#include <clutter/clutter.h>	/* clutter Library @ www.clutter.org */
#include <gst/gst.h>	/*Gstreamer Libary @ http://gstreamer.freedesktop.org/*/
#include <clutter-gst/clutter-gst.h>	/* clutter Library @ www.clutter.org */
#include <gtk/gtk.h>	/* Gimp Toolkit Library @ gimp.org */

#include <gst/rtsp-server/rtsp-server.h>	/* Gstreamer Library for Real Time Streaming Protocol */

/* Json Parser Library By: Copyright (2012) Daser Retnan @ http://github.com/retnan/surulib/  	*/
#include "surulib.h"

/* BSD socket Library */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


/* Dimension of Window Display */
#define WIN_WIDTH		750	
#define WIN_HEIGHT		600

/* Dimension of Video Display */
#define STAGE_WIDTH		640	
#define STAGE_HEIGHT		480	

#define CLIENT_PORT "1981" // the port client will be connecting to

/* Server Connection Details */
#define PORT "1980" // the port client will be connecting to
#define SERVER_IP "127.0.0.1"	//server IP address
#define MAXDATASIZE 500 // Max number of bytes expected from server
#define INTRO "Welcome to Blessing Menina's Video Conferencing Application"
#define WINTITLE "Video Conferencing Application"


typedef struct VID_T
{
	ClutterTexture* texture;
	GstPipeline* pipeline;
	GstElement* source;
	GstElement* decode;
	GstElement* colorspace;
	ClutterGstVideoSink* sink;
} VID;

/* This structure contains the components of the window container */

typedef struct iface
{
	ClutterActor *stage;
	GtkWidget *window;
	GtkWidget *clutter;
	ClutterActor* image;
	GtkWidget* txtip;
	GtkWidget* button;
	GtkWidget* label;
	ClutterTexture* videospace;
	gboolean active;
	pthread_t* server_t_id;
	int server_conn_id;	/* Connection ID */
	char rtspuri[50];
	char username[50];
} GUI;

/* here, we make the interface structure global so we can manipulate it from everywhere and also local to this file*/
static GUI* interface = NULL;

/* 
This function is used for loading network video stream using the rtsp protocol. 
the URI shared by the target client
*/

VID* loadStream( char* uri, char* id )
{
	char* gstName = (char*)malloc( sizeof( char ) * 64 );
	
	VID* video = (VID*)malloc( sizeof( VID ) );
	video->texture = CLUTTER_TEXTURE( clutter_texture_new() );
	
	sprintf( gstName, "pipeline%s", id );
	video->pipeline = GST_PIPELINE( gst_element_factory_make( "playbin2", gstName ) );
	g_object_set( G_OBJECT( video->pipeline ), "uri", uri, NULL );
	
	video->sink = (ClutterGstVideoSink*)clutter_gst_video_sink_new( video->texture );
	g_object_set( G_OBJECT( video->pipeline ), "video-sink", video->sink, NULL );
	
	gst_element_set_state( GST_ELEMENT( video->pipeline ), GST_STATE_PLAYING );
	
	free( gstName );
	
	return video;
}

/* Let's Free the memory used by the gstreamer framework */
void unloadCamera( VID* video )
{
	gst_object_unref( video->pipeline );
	free( video );
}

/* This shows what (Your Face) will be transmited to the target client. */
VID* loadCamera( char* device, char* id )
{
	char* gstName = (char*)malloc( sizeof( char ) * 64 );

	VID* video = (VID*)malloc( sizeof( VID ) );
	video->texture = CLUTTER_TEXTURE( clutter_texture_new() );
	
	sprintf( gstName, "pipeline%s", id );
	video->pipeline = (GstPipeline*)gst_pipeline_new( gstName );
	perror("gstreammer\n");
	
	sprintf( gstName, "source%s", id );
	
	video->source = gst_element_factory_make( "v4l2src", gstName );

	g_object_set( G_OBJECT( video->source ), "device", device, NULL );

	sprintf( gstName, "colorspace%s", id );
	video->colorspace = gst_element_factory_make( "ffmpegcolorspace", gstName );

	video->sink = (ClutterGstVideoSink*)clutter_gst_video_sink_new( video->texture );

	gst_bin_add_many( GST_BIN( video->pipeline ), video->source, video->colorspace, GST_ELEMENT( video->sink ), NULL );

	gst_element_link_many( video->source, video->colorspace, GST_ELEMENT( video->sink ), NULL );
	gst_element_set_state( GST_ELEMENT( video->pipeline ), GST_STATE_PLAYING );
	
	free( gstName );
	
	return video;
}


static gboolean connect_to_video_feed (GtkButton *button, gpointer user_data)
{

	/*
	Flags an actor to be hidden. A hidden actor will not be rendered on the stage.
	Now, let's hide the Initial Image on screen
	*/

	clutter_actor_hide(interface->image);

	VID* cam1 = loadCamera( "/dev/video0", "_cam1");
	//VID* cam1 = loadStream( "rtsp://127.0.0.1:8554/test", "_cam1");
	clutter_container_add_actor( CLUTTER_CONTAINER(interface->stage), CLUTTER_ACTOR(cam1->texture) );
	clutter_actor_show( CLUTTER_ACTOR(cam1->texture ));
	interface->videospace = cam1->texture;
	interface->active = TRUE;
	gtk_button_set_label((GtkButton *)interface->button,"DISCONNECT VIDEO FEED");

  return TRUE;
}

/* this timeout is periodically run to clean up the expired sessions from the
 * pool. This needs to be run explicitly currently but might be done
 * automatically as part of the mainloop. */

static gboolean timeout (GstRTSPServer * server, gboolean ignored)
{
/* Taken from rtsp Project Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com> */

  GstRTSPSessionPool *pool;

  pool = gst_rtsp_server_get_session_pool (server);
  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);

  return TRUE;
}


static gboolean broadcast_video (const char * rtsp_uri)
{

/* Taken from rtsp Project Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com> */

/* Modified for Use here */

	GMainLoop *loop = g_main_loop_new (NULL, FALSE);
	char buf[30];

	/* create a stream server instance */
	GstRTSPServer *server = gst_rtsp_server_new ();

/* get the mapping for this server, every server has a default mapper object
 that be used to map uri mount points to media factories */
	
	GstRTSPMediaMapping *mapping = gst_rtsp_server_get_media_mapping (server);

/*make a media factory for a test stream. The default media factory can use gst-launch syntax to create pipelines.
* any launch line works as long as it contains elements named pay%d. Each element with pay%d names will be a stream */

	GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new ();

/* TODO: IP must be specified for a networked environment */

	gst_rtsp_media_factory_set_launch (factory, "( "
	"v4l2src device=/dev/video0 ! ffmpegcolorspace ! "
	"video/x-raw-yuv,width=352,height=288,framerate=20/1 ! "
	"videoscale ! x264enc byte-stream=true bitrate=300 ! rtph264pay name=pay0 pt=96 "
	"pulsesrc ! audio/x-raw-int,rate=8000 ! "
	"alawenc ! rtppcmapay name=pay1 pt=97 " ")");

	/*
	strcpy(buf,interface->rtspuri);
	strcat(buf,":");
	strcat(buf,CLIENT_PORT);
	strcat(buf,"/");
	strcat(buf,interface->username);
	*/

	bzero(buf,29);
	strcat(buf,"/");
	strcat(buf,interface->username);

	/* attach the test factory to the /test url make it the interface->username*/
	gst_rtsp_media_mapping_add_factory (mapping, "/test", factory);
	//gst_rtsp_media_mapping_add_factory (mapping, buf, factory);

	g_object_unref (mapping);

	/* attach the server to the default maincontext */
	if (gst_rtsp_server_attach (server, NULL) == 0){
	fprintf(stderr,"failed to attach the server\n");
	return FALSE;
	}

	 /* add a timeout for the session cleanup */
	g_timeout_add_seconds (2, (GSourceFunc) timeout, server);

	 /* start serving, this never stops */
	g_main_loop_run (loop);

	/* I doubt if this function ever returns, hence running in a seperate thread */
  return TRUE;
}

static gboolean init_stream_server(char * rtsp)
{
	pthread_t* thread_id = (pthread_t*) malloc(sizeof(pthread_t));	/* still in memory as long it'sn't free()ed */

/* static gboolean broadcast_video (const char * rtsp_uri) */

	/* Here, we run this code (the server) concurently (in a separate thread) */
	pthread_create (thread_id, NULL, &broadcast_video, NULL);	/* TODO Ensure you pass the rtsp uri */
	/* This will be helpful should we want to kill the server's thread */
	interface->server_t_id = thread_id;	/* Later Examine the thread_id to know if stream server is alive */

	if(*thread_id){
	strcpy(interface->rtspuri, rtsp);
	return TRUE;
	}

	return FALSE;
}

static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int get_socket_fd(int * error)
{
	int sockfd, rv;  
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(SERVER_IP, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        *error = -1;	/* Error */
	return -1;
	}

    // loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {

		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
		perror("client: socket");
		continue;
        	}//FI

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		close(sockfd);
		perror("client: connect");
		continue;
		}//FI

	break;

	}//NEXT p

	if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
	*error = -1;	/* Error */
	return -1;
	}//FI

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

	fprintf(stdout,"client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // free struct

return sockfd;
}

static gboolean register_rtsp(const char * username, const char * rtspuri, int * server_conn_id)
{
/* Client-Server code here */
	int sockfd, nbytes;
	char *buf = NULL;	/* Request Buffer */
	int error_no = 0;
	/* To be used by surulib */
	database *parsed_json = NULL;
	char sid[30];

	buf = (char *) calloc(sizeof(char), MAXDATASIZE);
	
/* Send Request To Server Now, Since we are connected */

	sockfd = get_socket_fd(&error_no);

	if(!sockfd){
	*server_conn_id = -1;	/* Error to caller*/
	return FALSE;
	}

/* Cook up the json request */

/* Example Request
*		{"command":"register_rtsp_address","username":"blessing","rtspuri":"192.168.50.1/me"}
*/
	sprintf(buf,"{\"command\":\"register_rtsp_address\",\"username\":\"%s\",\"rtspuri\":\"%s\"}",username,rtspuri);

	if (send(sockfd, buf, strlen(buf), 0) == -1){
	perror("Send-Error:");
	*server_conn_id = -1;	/* Error to caller*/
	return FALSE;
	}
	
	bzero(buf,MAXDATASIZE - 1); /* Nullify the entire memory */	
	
	if((nbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("Recv-Error");
        *server_conn_id = -1;	/* Error to caller*/
	return FALSE;
	}

	buf[nbytes] = '\0';	/* Expecting the worst */

	
	fprintf(stdout,"Client: received '%s'\n", buf);

	close(sockfd);	/* Close socket */

/* Analyse buf with surulib */

	parsed_json = (database*)SURU_initJson(buf);

	if(!parsed_json){
	SURU_free(parsed_json);
	*server_conn_id = -1;	/* Error to caller*/
	return FALSE;
	}

/* Expected Sample json reply from Video Conf. server=
*	{"command":"register","sid":"568895"}  
*			OR 
*	{"command":"error","reason":"Failure"}
*/

	if(strcmp((char *)SURU_getElementValue(parsed_json,"command"),"register") == 0){
	  strcpy(sid,(char *)SURU_getElementValue(parsed_json,"sid"));
	  SURU_free(parsed_json);	/* Tell surulib to free its memory */
	  *server_conn_id = atoi(sid); /* atoi function converts char to int (defined in #include <stdlib.h>) */
	
		if(*server_conn_id > 1){
		return TRUE;
		}

	}else{
	//An error occured.....echo to stderr
		if(strcmp((char *)SURU_getElementValue(parsed_json,"command"),"error") == 0){
		fprintf(stderr,"SERVER ERROR: %s\n",(char *)SURU_getElementValue(parsed_json,"reason"));
		}else{
		fprintf(stderr,"An unknown error just occured when registering\n");
		}
	SURU_free(parsed_json);	/* Tell surulib to free its memory */
	}

return FALSE;
}

static char * get_users_list(const int server_conn_id)
{
/* Client-Server code here */
	int sockfd, nbytes;
	char *buf = NULL;	/* Request Buffer */
	int error_no = 0;

	/* To be used by surulib */
	database *parsed_json = NULL;
	char result[MAXDATASIZE];

	buf = (char *) calloc(sizeof(char), MAXDATASIZE);
	
/* Send Request To Server Now, Since we are connected */

	sockfd = get_socket_fd(&error_no);

	if(!sockfd){
	return FALSE;
	}

/* Example
*	{"command":"list_of_clients","sid":"568895"}
*/

	sprintf(buf,"{\"command\":\"list_of_clients\",\"sid\":\"%d\"}",server_conn_id);


	if (send(sockfd, buf, strlen(buf), 0) == -1){
	perror("Send-Error:");
	return FALSE;
	}

	if((nbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("Recv-Error");
	return FALSE;
	}

	buf[nbytes] = '\0';

	fprintf(stdout,"Client: received '%s'\n", buf);

	close(sockfd);

/* Analyse result with surulib */

/*Sample format expected from videoconf server
*
*{"command":"list_of_clients","result":"Online Users: blah blah blash"}
*		OR
*{"command":"error","reason":"Register RTSP First"}
*/
	if(strcmp((char *)SURU_getElementValue(parsed_json,"command"),"list_of_clients") == 0){
	  strcpy(result,(char *)SURU_getElementValue(parsed_json,"result"));
	  SURU_free(parsed_json);	/* Tell surulib to free memory */
	}else{
	//An error occured.....echo to stderr
		if(strcmp((char *)SURU_getElementValue(parsed_json,"command"),"error") == 0){
		fprintf(stderr,"SERVER ERROR: %s\n",(char *)SURU_getElementValue(parsed_json,"reason"));
		}else{
		fprintf(stderr,"An unknown error just occured when registering\n");
		}
	}
return NULL;
}

static char * get_online_users()
{
	int server_conn_id = 0;
	char * online_users = NULL;
	
	if(interface->server_t_id){	/* Our thread still runing */

/* MAXDATASIZE = 500! more than 500 chars (bytes) may crash the server TODO: realloc in due course*/

	online_users = (char *) malloc(sizeof(char) * MAXDATASIZE);	
		if(!interface->server_conn_id){		/* We dont have a session ID */

			//register our RTSP address and fetch on

			if(register_rtsp(interface->username,interface->rtspuri,&server_conn_id)){
			interface->server_conn_id = server_conn_id;
		
			strcpy(online_users, get_users_list((int)interface->server_conn_id)); /* can return a NULL pointer */

				if(!online_users){
				return online_users;
				}
			}
		}else{
			strcpy(online_users, get_users_list((int) interface->server_conn_id)); /* can return a NULL pointer */

			if(!online_users){
			return online_users;
			}
		}
	}else{
	fprintf(stderr,"CALL init_stream_server(char * rtsp) FIRST");
	}

	return NULL;
}

static gboolean process_stage_request(ClutterStage *stage, ClutterEvent *event, gpointer user_data)
{

/* Toggle partial fullscreen */

static int ct = 0;
int w = 0;	/* width of window */
int h = 0;	/* Height of window */
	if(ct == 0){
	gtk_widget_hide(interface->button);
	gtk_widget_hide(interface->txtip);
	gtk_widget_hide(interface->label);
	clutter_stage_hide_cursor ((ClutterStage *)interface->stage);

		if(interface->active){
		gtk_window_get_size((GtkWindow *)interface->window,&w,&h);
		clutter_actor_set_size((ClutterActor *)interface->videospace,w,h);
		}
	ct++;
	}else{
	gtk_widget_show(interface->button);
	gtk_widget_show(interface->txtip);
	gtk_widget_show(interface->label);
	clutter_stage_show_cursor((ClutterStage *)interface->stage);
	ct--;
	}

  return TRUE; /* return false to Stop further handling of this event. */
}

int main(int argc, char *argv[])
{

	ClutterColor stage_color = { 0x00, 0x00, 0x00, 0x00 }; /* blue */

	gtk_clutter_init (&argc, &argv);
	clutter_gst_init( &argc, &argv );	/* Trust me! Gstreamer won't work without it */

	interface = (GUI *) malloc(sizeof(GUI));
	interface->active = FALSE;

		if(interface == NULL){
		perror("The Process is out of memory");
		return 2;
		}

  /* Create the window and some child widgets: */

	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size((GtkWindow*)window,WIN_WIDTH,WIN_HEIGHT);
	gtk_window_set_resizable((GtkWindow*)window,FALSE);
	gtk_window_set_title ((GtkWindow*)window,WINTITLE);

	interface->window = window;

  /* Create a Container to house our widgets */

	GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show (vbox);

  /* Create the clutter widget: */

	GtkWidget *clutter = (GtkWidget *) gtk_clutter_embed_new ();
	gtk_box_pack_start (GTK_BOX (vbox), clutter, TRUE, TRUE, 0);
	gtk_widget_show (clutter);

	interface->clutter = clutter;
  /* creating the label */

	GtkWidget* label = gtk_label_new(INTRO);
	gtk_box_pack_end (GTK_BOX (vbox), label, TRUE, FALSE, 0);
	gtk_widget_show(label);

	interface->label = label;

  /* Creating the IP address Text Box */

	GtkWidget* txtip= gtk_entry_new();
	gtk_entry_set_text ((GtkEntry*) txtip,"Enter video feed IP");
	gtk_box_pack_end (GTK_BOX (vbox), txtip, FALSE, FALSE, 0);
	gtk_widget_show(txtip);

	interface->txtip = txtip;	
	
  /* Creating the connect button */

	GtkWidget* button = gtk_button_new_with_label ("Connect");
	gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0); /*add button to the container */
	gtk_widget_show (button); /* make it visible on the window */
	g_signal_connect (button, "clicked", G_CALLBACK (connect_to_video_feed), NULL);

	interface->button = button;

  /* Stop the application when the window is closed: */
	g_signal_connect (window, "hide",G_CALLBACK (gtk_main_quit), NULL);

  
  /* Working infinitly with the clutter widget (DO SOME SHITS HERE WITH CLUTTER)
   * Set the size of the widget, 
   * because we should not set the size of its stage when using GtkClutterEmbed.
   */ 
	gtk_widget_set_size_request (clutter, STAGE_WIDTH, STAGE_HEIGHT);

  /* Get the stage and set its size and color: */

	ClutterActor *stage = (ClutterActor *)gtk_clutter_embed_get_stage (clutter);
	clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

	interface->stage = stage;

	ClutterActor *actor_img = clutter_texture_new_from_file("slamdok.ppm", NULL);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), actor_img);
	clutter_actor_show(actor_img);

	interface->image = actor_img;
	interface->server_conn_id = 0;
	strcpy(interface->username,"Blessing");	/* Change this for another client otherwise obtain from UI*/
	//strcpy(interface->username,argv[1]);	/* Change this for another client otherwise obtain from UI*/
	//strcpy(interface->rtspuri,argv[2]);	/* Change this for another client otherwise obtain from UI*/
	strcpy(interface->rtspuri,"192.168.50.1");	/* Change this for another client otherwise obtain from UI*/

  /* Show the stage: */
	clutter_actor_show (stage);

  /* Connect a signal handler to handle mouse clicks and key presses on the stage: */ 
	g_signal_connect (stage, "button-press-event", G_CALLBACK (process_stage_request), NULL);

  /* Show the window: */
	gtk_widget_show (GTK_WIDGET (window));

  /* Start the main loop, so we can respond to events: */
	gtk_main ();

	return 0;

}
