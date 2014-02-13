/*#define FALSE 0
#define TRUE 1
*/
#define SURUMAX 6000
#define PRIVATE 
#define PUBLIC 

typedef struct Database{
char data[6000];
char value[6000];
struct Database *next;
}database;


/**************************************************************************************************************
*
*		Copyright 2012 Daser Retnan (CodePoet)	dasersolomon@gmail.com
*
*			Development @ http://github.com/retnan/surulib/
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

