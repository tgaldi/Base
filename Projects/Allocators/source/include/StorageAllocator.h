#pragma once

#include <cstddef>
#include <stdlib.h>
#include <stdio.h>

// K&R Storage Allocator, Section 8.7

// Unions provide a way to manipulate different kinds of head in a single
// area of storage, without embedding any machine-dependent information.
// The compiler keeps track of the size and aligment requirements.

// In effect, a union is a structure in which all members have offset zero from
// the base, the structure is big enough to hold the "widest" member, and the
// alignment is approriate for all of the types in the union.
typedef max_align_t Align;

// Header stores size of its memory block (actual free memory) 
// and a pointer to next free block's header.
union header
{
    struct
    {
        union header* next; // pointer to next block
        size_t size;        // size of this block
    } head;
    Align x;
};
typedef union header Header;

// In the absense of explicit initialization, static variables 
// are guaranteed to be initialized to zero.
static Header g_base;

// The search for a free block of adequate size begins at g_freePtr,
// where the last block was found; this strategy helps keep the list homogeneous.
static Header* g_freePtr = nullptr;

static size_t g_totalMemory;
size_t GetFreeMemory;


// function prototypes
static Header* mem_request( size_t );
static void mem_release( void* );

void* mem_alloc( size_t nbytes )
{
    // This is a standard formula of unit calculation with integer rounding: ( (nunits - 1) / unitSize ) + 1
    // We calculate the number of memory blocks needed (including size of header). 
    // All blocks are multiples of the header size to maintain alignment. 
    // Subtract 1 for case where number of bytes needed == sizeof( Header ),
    // which will return 1 extra block of memory than needed.
    // ex: ( nbytes( 8 ) + header( 8 ) / header( 8 ) ) + 1 => 2 + 1 => 3 ( 1 extra )
    size_t blocks = ( ( nbytes + sizeof( Header ) - 1 ) / sizeof( Header ) ) + 1;

    // initialize base of memory block linked-list
    if( g_freePtr == nullptr )
    {
        g_base.head.next = &g_base;
        g_base.head.size = 0;

        g_freePtr = &g_base;
    }

    // prevPtr tracks last free memory header
    Header* prevPtr = g_freePtr;

    // currPtr searches for free memory
    Header* currPtr = g_freePtr->head.next;

    // "Next-fit search": Search for a free block of memory that is big-enough for the request in a list of addresses.
    // Searches begin from previous successful position. This allows skipping blocks of a smaller size at the beginning of the heap.
    for( ; ; prevPtr = currPtr, currPtr = currPtr->head.next )
    {
        // space is available in current free block
        if( currPtr->head.size >= blocks )
        {
            // memory request fits exactly in current free block
            if( currPtr->head.size == blocks )
                prevPtr->head.next = currPtr->head.next;
            else
            {
                // Request is smaller than current free block size,
                // so move forward by excess block size to clip the memory from the tail end.
                currPtr->head.size -= blocks;
                currPtr += currPtr->head.size;
                currPtr->head.size = blocks;

                /*------------------------------------------------------------------------
                mem_alloc( 2 ) => ( ( 2 + 16 - 1 ) / 16 ) + 1 => (17 / 16) + 1 => 2 blocks
                currPtr == g_freePtr
                    mem_request( 2 ) => NALLOC * sizeof(Header) => 1024 * 16 => 1024 blocks
                    {base}{1}[1023] - 2 => {base}{1}[1021]{1}[1]
                    g_freePtr = {base}

                currPtr = g_freePtr->next = {1}[1021]
                currPtr + 1021 => {1}[1]
                return currPtr + 1 => [1]
                ------------------------------------------------------------------------*/
            }

            // maintain pointer to free block preceding returned block for next search
            g_freePtr = prevPtr;

            // add 1 to skip header when returning memory
            return (void*)( currPtr + 1 );
        }

        // The list is circular; the last block points to the first.
        // If the search wraps around the list, more memory is requested.
        if( currPtr == g_freePtr )
        {
            if( ( currPtr = mem_request( blocks ) ) == nullptr )
                return nullptr;
        }
    }

}

#define NALLOC 1024
static Header* mem_request( size_t blocks )
{
    if( blocks < NALLOC )
        blocks = NALLOC;

    void* memPtr = malloc( blocks * sizeof( Header ) );
    if( memPtr )
    {
        printf( "... (malloc) claimed %zu blocks.\n", blocks );
        g_totalMemory += blocks;

        Header* insrtPtr = (Header*)memPtr;
        insrtPtr->head.size = blocks;

        // add memory to block list
        mem_release( (void*)( insrtPtr + 1 ) );

        // return pointer to second to last block
        return g_freePtr;
    }
    else
    {
        fprintf( stderr, "ERROR: out of memory.\n" );
        return nullptr;
    }
}

// convert pointer into double pointer to address of passed-in pointer for zero'ing
#define mem_free(p) _mem_free((void **)&p)
void _mem_free( void** memory )
{
    if( *memory == nullptr )
        return;

    mem_release( *memory );

    *memory = nullptr;
}

void mem_release( void* blockPtr )
{
    if( blockPtr == nullptr )
        return;

    // Header of memory to be inserted back into free list
    Header* insrtPtr = (Header*)blockPtr - 1;

    if( insrtPtr->head.size == 0 || insrtPtr->head.size > g_totalMemory )
        return;

    // Start of free list (not necessarily the lowest address).
    // If we started at g_base each time, we would drain the beginning of the list of larger blocks (because they satisfy requests more easily).
    // So the list would start with lots of smaller blocks, and requests for medium or larger sizes
    // would have to traverse more of the list to get to something suitable.
    Header* currPtr = g_freePtr;

    // Search free list for a location to insert the memory.
    // Search is done by moving currPtr, understanding that memory addresses increase in the list.
    // The location will either fall between two addresses, or at the start or end of the list.
    // ( insrtPtr > currPtr && insertPtr < currPtr->head.next ): insert address is between two blocks
    // ( currPtr >= currPtr->head.next ): the search is at the end of the list
    // ( insrtPtr > currPtr || insrtPtr < currPtr->head->next ): insert address is past the highest address or before lowest
    for( ; !( insrtPtr > currPtr && insrtPtr < currPtr->head.next ); currPtr = currPtr->head.next )
    {
        if( currPtr >= currPtr->head.next && ( insrtPtr > currPtr || insrtPtr < currPtr->head.next ) )
            break;
    }

    // The address range of the insert block is adjacent (higher) than the current free memory block,
    // so combine it with the NEXT free block
    if( insrtPtr + insrtPtr->head.size == currPtr->head.next )
    {
        insrtPtr->head.size += currPtr->head.next->head.size; // combine size
        insrtPtr->head.next = currPtr->head.next->head.next;  // move the next free address up
    }
    else insrtPtr->head.next = currPtr->head.next; // otherwise the block is not adjacent, so insert it as the left/lower address

    // The address of the insert block is the same as the end of the block previous to it,
    // so they can be combined (left->right | lower->upper).
    if( currPtr + currPtr->head.size == insrtPtr )
    {
        currPtr->head.size += insrtPtr->head.size; // combine size
        currPtr->head.next = insrtPtr->head.next;  // move the next free address up
    }
    else currPtr->head.next = insrtPtr; // otherwise the block is not adjacent, so insert it as the next/upper address

    // move the free pointer to the block before the insertion
    g_freePtr = currPtr;
}

void Visualize( const char* msg )
{
    printf( "--- Free list after \"%s\":\n", msg );

    if( g_freePtr == NULL )
    {
        printf( "\tList does not exist\n\n" );
        return;
    }

    if( g_freePtr == g_freePtr->head.next )
    {
        printf( "\tList is empty\n\n" );
        return;
    }

    printf( "  base: %10p -->  ", (void*)&g_base );
    printf( "  ptr: %10p size: %-3zu -->  ", (void*)g_freePtr, g_freePtr->head.size );

    Header* currPtr = g_freePtr;
    while( currPtr->head.next > g_freePtr )
    {
        currPtr = currPtr->head.next;
        printf( "ptr: %10p size: %-3zu -->  ", (void*)currPtr, currPtr->head.size );
    }
    printf( "end\n\n" );
}

size_t GetFreeBlockCount()
{
    if( g_freePtr == NULL )
        return 0;

    Header* currPtr = g_freePtr;
    size_t count = currPtr->head.size;

    while( currPtr->head.next > currPtr )
    {
        currPtr = currPtr->head.next;
        count += currPtr->head.size;
    }

    return count;
}
