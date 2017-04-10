#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "phoenix.h"


int main(){


 printf("basic testing of phoenix functionality\n");
 assert(px_init(1) == 0);

 //test: create object
 px_obj obj1;
 assert(!px_create("foo",100, &obj1));
 assert(obj1.data && obj1.size == 100);


 //populate memory with some data
 strncpy((char *)obj1.data, "Some wierd text",10);

 //test: get object
  px_obj obj2;
  assert(!px_get("foo",-1 ,&obj2));
  assert(obj2.size == 100);
  assert(!strncmp((char *)obj1.data,(char *)obj2.data,100));

 //test: commit object to nvm
 px_obj obj3;
 assert(px_commit("foo",2) == 0);


 //test: get a versioned object
 px_obj obj4;
 assert(!px_get("foo",1,&obj4));
 assert(obj4.size == 100);
 assert(!strncmp((char *)obj1.data,(char *)obj4.data,100));
 //TODO: compare actual data content

 return 0;
}
