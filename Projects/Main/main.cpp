#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "LinkedList.h"
#include "StorageAllocator.h"

int main( int argc, char** argv )
{
    //LinkedList<int>* list = new LinkedList<int>( 1 );

    //list->Append( 1 );
    //list->Append( 2 );
    //list->Append( 3 );
    //auto node = list->Append( 4 );
    //list->Append( 5 );

    //list->Remove( node );
    //node = list->Append( 6 );
    //list->Append( 7 );
    //list->Remove( node );

    //delete( list );

    char* pc, * pcc, * pccc, * ps;
    long* pd, * pdd;
    int dlen = 100;
    int ddlen = 50;

    Visualize( "start" );


    /* trying to fragment as much as possible to get a more interesting view */

    /* claim a char */
    if( ( pc = (char*)mem_alloc( sizeof( char ) ) ) == NULL )
        return -1;

    /* claim a string */
    if( ( ps = (char*)mem_alloc( dlen * sizeof( char ) ) ) == NULL )
        return -1;

    /* claim some long's */
    if( ( pd = (long*)mem_alloc( ddlen * sizeof( long ) ) ) == NULL )
        return -1;

    /* claim some more long's */
    if( ( pdd = (long*)mem_alloc( ddlen * 2 * sizeof( long ) ) ) == NULL )
        return -1;

    /* claim one more char */
    if( ( pcc = (char*)mem_alloc( sizeof( char ) ) ) == NULL )
        return -1;

    /* claim the last char */
    if( ( pccc = (char*)mem_alloc( sizeof( char ) ) ) == NULL )
        return -1;


    printf( "\n" );
    mem_free( pccc );
    /*      bugged on purpose to test mem_free(NULL) */
    mem_free( pccc );
    Visualize( "mem_free(the last char)" );

    mem_free( pdd );
    Visualize( "mem_free(lot of long's)" );

    mem_free( ps );
    Visualize( "mem_free(string)" );

    mem_free( pd );
    Visualize( "mem_free(less long's)" );

    mem_free( pc );
    Visualize( "mem_free(first char)" );

    mem_free( pcc );
    Visualize( "mem_free(second char)" );


    /* check memory condition */
    size_t freemem = GetFreeBlockCount();
    printf( "\n" );
    printf( "--- Memory claimed  : %zu blocks (%zu bytes)\n",
        g_totalMemory, g_totalMemory * sizeof( Header ) );
    printf( "    mem_free memory now : %zu blocks (%zu bytes)\n",
        freemem, freemem * sizeof( Header ) );
    if( freemem == g_totalMemory )
        printf( "    No memory leaks detected.\n" );
    else
        printf( "    (!) Leaking memory: %zu blocks (%zu bytes).\n",
            ( g_totalMemory - freemem ), ( g_totalMemory - freemem ) * sizeof( Header ) );

    printf( "// Done.\n\n" );

    system( "pause" );
    return 0;
}