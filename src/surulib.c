/**************************************************************************************************************
*
*		Copyright 2012 Daser Retnan (CodePoet)	dasersolomon@gmail.com
*
*			Development @ https://github.com/retnan/surulib/
*
**************************************************************************************************************
				SURU JSON PARSER (surulib)
***************************************************************************************************************
*  	SURU is a Portable and a <em>BASIC</em> Json Parser Named after a 500L Computer Science
*	Abubakar Tafawa Balewa University, Bauchi, Nigeria Who Died in
*	a Bomb Blast Caused by an Islamic suicide bomber during a Church Service @
*	living Faith Church Bauchi on 3rd June, 2012. RIP SURU, etal.
***************************************************************************************************************
*
*  	Licensed under the Apache License, Version 2.0 (the "License");
*  	you may not use this file except in compliance with the License.
*  	You may obtain a copy of the License at
*
*       	http://www.apache.org/licenses/LICENSE-2.0
*
*  	Unless required by applicable law or agreed to in writing, software
*  	distributed under the License is distributed on an "AS IS" BASIS,
*  	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  	See the License for the specific language governing permissions and
*  	limitations under the License.
*
**************************************************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include "surulib.h"

PUBLIC database * SURU_initJson(char * filename);
PUBLIC char * SURU_getElementValue(const database * db, char * data);
PRIVATE void SURU_appendDataValue(database * db, char * data, char * value);	/*Not impemented*/
PUBLIC void * SURU_free(database * db);

PUBLIC void * SURU_free(database * db){
while(db){
	bzero(db,sizeof(db)-1);
	db = db->next;
}

free(db);
}


PUBLIC database * SURU_initJson(char * filename){
int count = 0; int zcount = 0; int begin = 0; int start = 0; int vcount = 0; int end = 0; int status = 0; int zzcount = 0;  int looptru = 0; int vend = 0; int vstart = 0; int gotchar = 0; int telvalue = 0;

FILE *fh = NULL;
int ch;
char bkup;

int llfirst = 1;

char data[SURUMAX];
char value[SURUMAX];
char source[SURUMAX];

bzero(source,SURUMAX - 1);

/* a proper argument validation */

/*if(strlen(filename) < 2){
fprintf(stderr,"invalid filename or Json String\n");
exit(1);
}*/

char *tcha = strrchr(filename,'.');

if(tcha != NULL){
	if(strcmp(".json",tcha) == 0){

	fh =  fopen((char *)filename,"r");	/*this should be *.json ortherwise we assume its a json string*/

		if(!fh){
		fprintf(stderr,"Error opening the file\n");
		return NULL;
		}

		if(fread(source,sizeof(char),SURUMAX,fh) != SURUMAX){
			
			if(feof(fh)){
			//this is good
			}else{
			fprintf(stderr,"Some errors occured\n");
			return NULL;
			}
		}
	}
}else{
strcpy(source,filename);//take the input string as json string instead of a file
}

database *db = NULL; //tail
database *head = NULL;
database *tmp = NULL;

db = malloc(sizeof(database));

if(db == NULL){
fprintf(stderr,"This Process is out of memory\n");
return NULL;
}

int srcount = 0;

	while((ch = source[srcount]) != '\0'){

		if(ch == '{' || ch == '['){	
			if(zcount == 1){
			}else{
			zcount++;
			}
		}

		if(looptru == 0){
			if(ch == '}' || ch == ']'){
				if(vcount == 0){
				break;
				}else{
				telvalue = 1;
				}
			}
		}

		if(zcount == 1){
			if(count == 0){
			begin = 1;
			}

			if(begin == 1){
				if(ch == '"' && end == 0){
				end = 1;
				start = 1;
				}else{
					if(start == 1){
						if(ch == '"'){
						data[count] = '\0';
						status = 1;
						begin = 0;
						srcount++;
						ch = source[srcount];
						}else{
						data[count] = ch;
						count++;
						}
					}
				}
			}


			if(status == 1){

				if(ch == '{' || ch == '['){
				zzcount++;
				looptru = 1;
				}

				if(ch == '}' || ch == ']'){
				zzcount--;
				}

				if(looptru == 1){
					
					if(zzcount == 0){
					value[vcount] = ch;
					looptru = 0;
					vcount++;
					value[vcount] = '\0';
					count = end = start = status = vcount = 0;
					if(db){
						if(llfirst){
						strcpy(db->data,data);
						strcpy(db->value,value);
						db->next = NULL;
						head = db;
						llfirst = 0;
						}else{
						tmp = malloc(sizeof(database));
							if(tmp == NULL){
							fprintf(stderr,"This Process is out of memory\n");
							return NULL;
							}
							strcpy(tmp->data,data);
							strcpy(tmp->value,value);
							tmp->next = NULL;
							db->next = tmp;
							db = tmp;
							tmp = NULL;
						}
					}else{
					fprintf(stderr,"Couldn't store Data=%s\tValue=%s\n",data,value);
					}
					bzero(data,SURUMAX - 1);
					bzero(value,SURUMAX - 1);
					}else{
					value[vcount] = ch;
					vcount++;
					}

				}

				
				if(looptru == 0){
					
						if((ch == '"' || isalnum(ch) != 0) && vend == 0){

							if(ch == '"'){
							gotchar = 1;
							}
							
							if(isalnum(ch) != 0){
							vend = vstart = 1;
							value[vcount] = ch;
							vcount++;
							}
						}else{
							if(vstart == 1){
								if(ch == '"' || ch == '\n' || ch == ',' || ch == '}'){

									if(ch == ',' && gotchar){
									value[vcount] = ch;
									vcount++;
									}else{
										if(bkup == '\\'){ /*escape \"*/
										value[vcount] = ch;
										vcount++;
										}else{
										value[vcount] = '\0';
										count = end = start = status = vcount = vend = vstart = gotchar = 0;
										if(db){
											if(llfirst){
											strcpy(db->data,data);
											strcpy(db->value,value);
											db->next = NULL;
											head = db; //backup the first itm
											llfirst = 0;
											}else{
											tmp = malloc(sizeof(database));
												if(tmp == NULL){
												fprintf(stderr,"This Process is out of memory\n");
												return NULL;
												}
											strcpy(tmp->data,data);
											strcpy(tmp->value,value);
											tmp->next = NULL;
											db->next = tmp;
											db = tmp;
											tmp = NULL;
											}
										}else{
										fprintf(stderr,"Couldn't store Data=%s\tValue=%s\n",data,value);
										}
										bzero(data,SURUMAX - 1);
										bzero(value,SURUMAX - 1);
										}
									}

									if(telvalue == 1){
									telvalue = 0;
									break;
									}
								}else{
									//if(ch != bkup){
									value[vcount] = ch;
									vcount++;
									//}
								}

							}

						}
				
				}//fi looptru

				
			}//fi value


		}//fi zcount
		

bkup = ch;
srcount++;
	}//wend

	if(head == NULL){
	//fprintf(stderr,"Unknown Error Occured\n");
	return NULL;
	}else{
	return head;
	}

}//SURU_initJson


PRIVATE void SURU_appendDataValue(database * db, char * data, char * value){


}

PUBLIC char * SURU_getElementValue(const database * db, char * data){

if(db == NULL){
	//fprintf(stderr,"SURU_NULL\n");
	return NULL;
	}else{

		while(db){

			if(strcmp(db->data,data) == 0){
			return (char *) db->value;
			}
		db = db->next;
		}
	}
return NULL;
}
