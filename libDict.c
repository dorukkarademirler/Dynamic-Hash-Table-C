#include<stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "libDict.h"

#define DEBUG
#define DEBUG_LEVEL 3

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

#define DICT_INIT_ROWS 1024 
#define DICT_GROW_FACTOR 2
#define ROW_INIT_ENTRIES 8
#define ROW_GROW_FACTOR 2 

#define PRIME1 77933 // a large prime
#define PRIME2 119557 // a large prime

/**
 * hash *c as a sequence of bytes mod m
 */
int dictHash(char *c, int m){
	int sum=0;
	while(*c!='\0'){
		int num = *c; 
		sum+= PRIME1*num+PRIME2*sum;
		c++;
	}
	if(sum<0)sum=-sum;
	sum = sum%m;
	return sum;
}

/**
 * Print the dictionary, 
 * level==0, dict header
 * level==1, dict header, rows headers
 * level==2, dict header, rows headers, and keys
 */
void dictPrint(Dict *d, int level){
	if(d==NULL){
		printf("\tDict==NULL\n");
		return;
	}
	printf("Dict\n");
	printf("\tnumRows=%d\n",d->numRows);
	if(level<1)return;

	for(int i=0;i<d->numRows;i++){
		printf("\tDictRow[%d]: numEntries=%d capacity=%d keys=[", i, d->rows[i].numEntries, d->rows[i].capacity);
		if(level>=2){
			for(int j=0;j<d->rows[i].numEntries;j++){
				printf("%s, ",d->rows[i].entries[j].key);
			}
		}
	printf("]\n");
	}
}

/**
 * Return the DictEntry for the given key, NULL if not found.
 * This is so we can store NULL as a value.
 */
DictEntry *dictGet(Dict *d, char *key){

	// find row
	int h = dictHash(key, d->numRows);
	// find key in row
	int counter = 0;
        char *curr = NULL;
        while (counter < d->rows[h].numEntries){
                curr = d->rows[h].entries[counter].key;
                if (strcmp(curr, key) == 0) {
                        return &d->rows[h].entries[counter];
                }
                counter++;
        }
        return NULL;
}

/**
 * Delete key from dict if its found in the dictionary
 * Returns 1 if found and deleted
 * Returns 0 otherwise
 */
int dictDel(Dict *d, char *key){
	// find row
	int h = dictHash(key, d->numRows);;

	#ifdef DEBUG
	//printf("dictDel(d,%s) hash=%d\n",key, h);
	//dictPrint(d,DEBUG_LEVEL);
	#endif
	int guard = 0;
	// find key
        // free key
        int counter = 0;
        while (counter < d->rows[h].numEntries) {
                if (strcmp(d->rows[h].entries[counter].key, key) == 0) {
                        free(d->rows[h].entries[counter].key);
                        // Move everything over
                        while (counter < d->rows[h].numEntries - 1){
                                d->rows[h].entries[counter] = d->rows[h].entries[counter + 1];
                                counter++;
                        }
                                d->rows[h].numEntries--;
                                guard = 1;
                }
                counter++;
                }

		if (d->rows[h].numEntries < (d->rows[h].capacity / 2) && d->rows[h].capacity > ROW_INIT_ENTRIES) {
        		int newCapacity = d->rows[h].capacity / ROW_GROW_FACTOR;
        		DictEntry *newMemo = realloc(d->rows[h].entries, newCapacity * sizeof(DictEntry));
        		if (newMemo != NULL) {
            			d->rows[h].entries = newMemo;
            			d->rows[h].capacity = newCapacity;
        		}
    		}

	int emptyRows = 0;
       		for (int i = 0; i < d->numRows; i++) {
                	if (d->rows[i].numEntries == 0) {
                        	emptyRows++;
                         }
                }

       	if (emptyRows > d->numRows / 2){
                //the new dictionary with 2 times the rows
                Dict *newDict = dictNew(d->numRows / ROW_GROW_FACTOR);
                for( int r = 0; r < d->numRows; r++){
                        for (int i = 0; i < d->rows[r].numEntries; i++) {

                                //newKey and newValue gets the values from d to put them into newDict
                                char *newKey = d->rows[r].entries[i].key;
                                void *newValue = d->rows[r].entries[i].value;
                                dictPut(newDict, newKey, newValue);
                                //Transfer Values
                        }
                }
                //was here
                for (int i = 0; i < d->numRows; i++) {
                        for(int x = 0; x < d->rows[i].numEntries; x++){
                                free(d->rows[i].entries[x].key);
                        }
                        free(d->rows[i].entries);
                }
                free(d->rows);
                *d = *newDict;
                free(newDict);
                dictPrint(d,3);
        }
        #ifdef DEBUG
        //dictPrint(d,DEBUG_LEVEL);
        #endif
	if (guard == 1){
		return 1;}
        return 0;
}


/**
 * put (key, value) in Dict
 * return 1 for success and 0 for failure
 */
int dictPut(Dict *d, char *key, void *value){
        int h = dictHash(key, d->numRows);

        #ifdef DEBUG
        //printf("dictPut(d,%s) hash=%d\n",key, h);
        //dictPrint(d,DEBUG_LEVEL);
        #endif

        // If key is already here, just replace value
        int counter = 0;
        char *curr = NULL;
        while(counter < d->rows[h].numEntries){
                curr = d->rows[h].entries[counter].key;
                if (strcmp(curr, key) == 0) {
                        d->rows[h].entries[counter].value = value;
                        return 1;
                }
                counter++;
        }

        // The part about extending an existing row
        if (d->rows[h].capacity <= d->rows[h].numEntries){
                        int newCapacity = d->rows[h].capacity * DICT_GROW_FACTOR;
                        DictEntry *newMemo = realloc(d->rows[h].entries, newCapacity * sizeof(DictEntry));
                        if (newMemo == NULL){
                                return 0;
                        }
                        else{
                                d->rows[h].entries = newMemo;
                                d->rows[h].capacity = newCapacity;
                        }
        }

	d->rows[h].entries[d->rows[h].numEntries].key = strdup(key);
        d->rows[h].entries[d->rows[h].numEntries].value = value;
        d->rows[h].numEntries++;

        //Checking if the rows should be expanded
        if (d->rows[h].numEntries >= 100){
                //the new dictionary with 2 times the rows
                Dict *newDict = dictNew(d->numRows * ROW_GROW_FACTOR);
                for( int r = 0; r < d->numRows; r++){
                        for (int i = 0; i < d->rows[r].numEntries; i++) {

                                //newKey and newValue gets the values from d to put them into newDict
                                char *newKey = d->rows[r].entries[i].key;
                                void *newValue = d->rows[r].entries[i].value;
				dictPut(newDict, newKey, newValue);
				//Transfer Values
                        }
                }
                //was here
                for (int i = 0; i < d->numRows; i++) {
                	for(int x = 0; x < d->rows[i].numEntries; x++){
                        	free(d->rows[i].entries[x].key);
                	}
                	free(d->rows[i].entries);
        	}
        	free(d->rows);
		*d = *newDict;
		free(newDict);
          //      dictPrint(d,3);
        }
        /**
         * This is a new key for this row, so we want to place the key, value pair
         * In python only immutables can be hash keys. If the user can change the key sitting
         * in the Dict, then we won't be able to find it again. We solve this problem here
         * by copying keys using strdup.
         *
         * At this point we know there is space, so copy the key and place it in the row
         * along with its value.
         */
        #ifdef DEBUG
        //printf("dictPut(d,%s) hash=%d\n",key, h);
        //dictPrint(d,DEBUG_LEVEL);
        #endif

        return 1;
}
/**
 * free all resources allocated for this Dict. Everything, and only those things
 * allocated by this code should be freed.
 */
void dictFree(Dict *d){
	for (int i = 0; i < d->numRows; i++) {
		for(int x = 0; x < d->rows[i].numEntries; x++){
			free(d->rows[i].entries[x].key);
			free(d->rows[i].entries[x].value);
		}
        	free(d->rows[i].entries);
        }
        free(d->rows);
        free(d);
}

/**
 * Allocate and initialize a new Dict. Initially this dictionary will have initRows
 * hash slots. If initRows==0, then it defaults to DICT_INIT_ROWS
 * Returns the address of the new Dict on success
 * Returns NULL on failure
 */
Dict * dictNew(int initRows){
	Dict *d=NULL;
        d = malloc(sizeof(Dict));
        if (d == NULL) {
                return NULL;
        }
	//Initializing Row Number 
	if (initRows==0){
		d->numRows = DICT_INIT_ROWS;
	}
	else{
		d->numRows = initRows;
	}
	//Allocate space for the amount of rows
	d->rows = malloc(d->numRows * sizeof(DictRow));
	if (d->rows == NULL){
		return NULL;
	}
	//Allocate space for each dictionary found in the different rows.
	for (int i=0; i < d->numRows; i++){
		d->rows[i].numEntries = 0;
        	d->rows[i].capacity = ROW_INIT_ENTRIES;
		d->rows[i].entries = malloc(sizeof(DictEntry) * d->rows[i].capacity);
		//added
		if (d->rows[i].entries == NULL){
			free(d->rows);
			free(d);
			return NULL;
		}
		for (int x=0; x < d->rows[i].capacity; x++){
			d->rows[i].entries[x].key = NULL;
			d->rows[i].entries[x].value = NULL;
		}
	}
	return d;
}

