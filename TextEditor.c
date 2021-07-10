#pragma GCC diagnostic ignored "-Wunused-result"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <time.h>

#define _STATE 2100 // Defines every how many operations the state should be saved

int stati; // If it's true states will be seved every _STATE operations

// Represents a node of the undo and redo stacks
typedef struct node_s{
	char c;
    int r1, r2,
        lastRowU, lastRowR;
	char **rowsU, **rowsR;
    struct node_s * next;
} node_t;

// Represents a state of the text
typedef struct state_s{
    char **table;
    int lastRow;
} state_t;

// Push a node in the undo or redo list
void push(node_t **top, node_t *node)
{
    node->next = *top;
    *top = node;
}

// Pop a node from the undo or redo list
node_t *pop(node_t **top)
{
    if(*top != NULL)
    {   
        node_t *node;
        node = *top;
        *top = (**top).next;
        node->next = NULL;
        return node;
    }
    return NULL;
}

// Empty teh undo or redo list
void empty(node_t **top, node_t **tail)
{
    node_t *node;
    *tail = NULL;
    while(*top != NULL)
    {
        node = *top;
        *top = (*top)->next;

        if(node->rowsU != NULL)
        {
            for(int i = 0; i <= node->r2 - node->r1 && i <= node->lastRowU - (node->r2 - node->r1 + 1); i++)
                free(node->rowsU[i]);
            free(node->rowsU);
        }

        if(node->rowsR != NULL)
        {
            for(int i = 0; i <= node->r2 - node->r1 && i <= node->lastRowR - (node->r2 - node->r1 + 1); i++)
                free(node->rowsR[i]);
            free(node->rowsR);
        }

        free(node);
    }
}

void saveState(),
    recoverState(int),
    change(int, int),
    delete(int, int),
    print(int, int), 
    undo(int),
    redo(int),
    changeU(node_t *),
    deleteU(node_t *),
    changeR(node_t *),
    deleteR(node_t *);

int lastRow = 0, // last text row
    nU = 0, // number of possible undo
    nR = 0, // number of possible redo
    nAlterations = 0, // number of changes from the last saved state
    nStates = -1; // number of saved states
char **table; // Text lines

state_t states[1000]; // The array of saved text states
node_t *topU = NULL, // Top of the undo stack
    *topR = NULL; // Top of the redo stack

void **memory; // Used as memory to instantiate nodes for undo and redo list instead of calling malloc every time
int memory_byte = 0; // Last used byte

int main()
{
    /*
        Instantiates a big amount of space is more efficent than reinstantiate the space every time because 
        leaves the memory management to the operating system which instantiates pages only when they are actually used
    */
    table = malloc(10000000);
    memory = malloc(1000000000);

    stati = 0;
    /* if(time(NULL) < 1599744911 + 300) // if it's before a specific time use states, otherwise no
         stati = 1;
    */

    int r1, r2, n, // user inputs: first row, second row, number
        countUR; // Number of gruped undo and redo: <0 redo; >0 undo
    char s[100], c; // user inputs: whole string, command


    if(stati == 1) saveState();

    while(1) // used to loop on user inputs
    {
        r1 = r2 = n = c = countUR = 0;
        
        do // used to group undo and redo
        {        
            fgets(s, 99, stdin);

            if(s[0] == 'q')
                goto end;
            char * v = strchr(s, ',');
            if( v != NULL)
                //change, delete, print
                sscanf(s, "%d,%d%c", &r1, &r2, &c);
            else
            {   //all
                sscanf(s, "%d%c", &n, &c);
                if(c == 'c' || c == 'd' || c == 'p')
                {
                    r1 = r2 = n;
                    n = 0;
                }
            }
            
            if(c == 'u')
                if(n <= nU)
                {
                    nU -= n;
                    nR += n;
                    countUR += n;
                }
                else
                {
                    nR += nU;
                    countUR  += nU;
                    nU = 0;
                }
            if(c == 'r')
                if(n <= nR)
                {
                    nR -= n;
                    nU += n;
                    countUR -= n;
                }
                else
                {
                    nU += nR;
                    countUR -= nR;
                    nR = 0;
                }
                

        }while(c == 'u' || c == 'r');

        if(countUR < 0)
            redo(-countUR);
        if(countUR > 0)
            undo(countUR);


        switch(c)
        {
            case 'c': //change
                change(r1, r2);
                nAlterations++;
                break;

            case 'd': //delete
                delete(r1, r2);
                nAlterations++;
                break;

            case 'p': //print
                print(r1, r2);
                break;
        }

        if(nAlterations == _STATE && stati)
        {
            nAlterations = 0;
            saveState();
        }
    }
    
    end:
    return 0;
}

// Save the currente text state into the states array
void saveState()
{
    nStates++;
    states[nStates].table = malloc(lastRow * sizeof(char *));
    states[nStates].lastRow = lastRow;
    memcpy(states[nStates].table, &table[0], lastRow * sizeof(char *));
}

// Restore a state from the states array
void recoverState(int n)
{
    lastRow = states[n].lastRow;
    memcpy(&table[0], states[n].table, lastRow * sizeof(char *));
    nStates = n - 1;
}

// Changes the lines from r1 to r2 reading from stdin
void change(int r1, int r2)
{
    node_t *node = (node_t *) malloc(sizeof(node_t));
    node->c = 'c';
    node->r1 = r1;
    node->r2 = r2;
    node->lastRowU = lastRow;
    node->rowsU = (char **) malloc((r2 - r1 + 1) * sizeof(char **));
    node->rowsR = (char **) malloc((r2 - r1 + 1) * sizeof(char **));

    for(int i = r1; i <= r2; i++)
    {
        fgets((char *) memory + memory_byte, 1024, stdin);
        int len = strlen((char *) memory + memory_byte);
        --len;
        *((char *) memory + memory_byte + len) = (char) 0;
        node->rowsU[i - r1] = table[i - 1];
        table[i - 1] = node->rowsR[i - r1] = (char *) memory + memory_byte;
        memory_byte += len + 1;
    }

    push(&topU, node);
    nU++;
    //empty(&topR); // Free the unused space slows the program
    topR = NULL;
    nR = 0;
    
    if(r2 > lastRow)
        lastRow = r2;
    node->lastRowR = lastRow;
}

// Deletes rows from r1 to r2
void delete(int r1, int r2)
{
    node_t *node = (node_t *) malloc(sizeof(node_t));
    /*node_t *node = (node_t *) memory + memory_byte;
    memory_byte += sizeof(node_t);*/
    node->c = 'd';
    node->r1 = r1;
    node->r2 = r2;
    node->lastRowU = lastRow;
    node->rowsU = (char **) malloc((r2 - r1 + 1) * sizeof(char **));
    /*node->rowsU = (char **) memory + memory_byte;
    memory_byte += (r2 - r1 + 1) * sizeof(char **);*/
    node->rowsR = NULL;

    memcpy(node->rowsU, &table[r1 - 1], (((r2 <= lastRow) ? r2 : lastRow) - r1 + 1) * sizeof(char *));

    if(r2 < lastRow)
    {
        memmove(&table[r1 - 1], &table[r2], (lastRow - r2) * sizeof(char *));
        lastRow = lastRow - (r2 - r1 + 1);
    }
    else
        if(r1 <= lastRow)
            lastRow = r1 - 1;

    push(&topU, node);
    nU++;
    //empty(&topR);
    topR = NULL;
    nR = 0;
    node->lastRowR = lastRow;
}

// Prints rows from r1 to r2
void print(int r1, int r2)
{
    for(int i = r1; i <= r2; i++)
    {
        if(i >= 1 && i <= lastRow)
            puts(table[i - 1]);
        else
            puts(".");
    }
}

// Undo n operations
void undo(int n)
{
    node_t *node;
    if(n >= nAlterations && stati)
    {
        n -= nAlterations;
        recoverState(nStates - (int) ((n / _STATE)));
        for(int i = 0; i < n - (n % _STATE) + nAlterations; i++)
        {
            node = pop(&topU);
            push(&topR, node);
        }
        n %= _STATE;
        nAlterations = _STATE;
    }

    for(int i = 1; i <= n; i++)
    {
        node = pop(&topU);
        switch (node->c)
        {
            case 'c':
                changeU(node);
                break;
            
            case 'd':
                deleteU(node);
                break;
            
            default:
                return;
        }
        nAlterations--;
    }

    if(nAlterations == _STATE && stati)
    {
        nAlterations = 0;
        saveState();
    }
    
}

// Redo n operations
void redo(int n)
{
    node_t *node;
    for(int i = 1; i <= n; i++)
    {
        node = pop(&topR);
        switch (node->c)
        {
            case 'c':
                changeR(node);
                break;
            
            case 'd':
                deleteR(node);
                break;
            
            default:
                return;
        }
        nAlterations++;
        if(nAlterations == _STATE && stati)
        {
            nAlterations = 0;
            saveState();
        }
    }
}

// Undo a change
void changeU(node_t *node)
{
    lastRow = node->lastRowU;

    memcpy(&table[node->r1 - 1], node->rowsU, (node->r2 - node->r1 + 1) * sizeof(char *));

    push(&topR, node);
}

// Redo a change
void changeR(node_t *node)
{
    lastRow = node->lastRowR;

    memcpy(&table[node->r1 - 1], node->rowsR, (node->r2 - node->r1 + 1) * sizeof(char *));

    push(&topU, node);
}

// Undo a delete
void deleteU(node_t *node)
{

    for(int i = lastRow; i >= node->r1; i--)
    {
        table[i - 1 + (node->r2 - node->r1 + 1)] = table[i - 1];
        if(i <= node->r2)
            table[i - 1] = node->rowsU[i - node->r1];
    }

    lastRow = node->lastRowU;

    push(&topR, node);
}

// Redo a delete
void deleteR(node_t *node)
{
    if(node->r2 < lastRow)
    {
        memmove(&table[node->r1 - 1], &table[node->r2], (lastRow - node->r2) * sizeof(char *));
        lastRow = lastRow - (node->r2 - node->r1 + 1);
    }
    else
        if(node->r1 <= lastRow)
            lastRow = node->r1 - 1;

    push(&topU, node);
}
